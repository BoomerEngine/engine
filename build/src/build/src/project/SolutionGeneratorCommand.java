package project;

import com.google.common.base.Stopwatch;
import generators.Generator;
import generators.PlatformType;
import generators.SolutionSetup;
import generators.SolutionType;
import library.Library;
import library.LibraryManager;
import project.dependencies.Dependency;
import project.dependencies.DependencyType;
import utils.GeneratedFilesCollection;
import utils.KeyValueTable;
import utils.ProjectException;
import utils.Utils;
import vs.VsSolutionSetup;
import cmake.CMakeSolutionSetup;

import java.nio.file.Path;
import java.util.*;
import java.util.stream.Collectors;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;

public class SolutionGeneratorCommand {

  public static Path DetermineOutputFolder(Path outputPath, PlatformType platformType, SolutionType solutionType, String generatorName) {
    return outputPath.resolve(String.format("%s.%s.%s/", platformType.toString().toLowerCase(), solutionType.toString().toLowerCase(), generatorName.toLowerCase()));
  }

  public static void GenerateSolution(ProjectManifest project, KeyValueTable options, SolutionType solutionType) {
    // load modules referenced by the project
    ModuleRegistry modules = new ModuleRegistry();
    for (Path p : project.moduleAbsolutePaths)
      modules.createModuleFromPath(p, 0);

    // add/download libraries to library respost
    Path libraryDownloadPath = project.tempDirectoryPath.resolve("libs/download/");
    Path libraryUnpackPath = project.tempDirectoryPath.resolve("libs/packages/");
    LibraryManager libs = new LibraryManager(libraryDownloadPath, libraryUnpackPath);
    for (ModuleManifest.ThirtPartyPackage url : modules.thirdPartyPackages)
      libs.addRemoteLibraryPackage(url);

    // resolve final platform and solution types
    PlatformType platformType = DeterminePlatform(options);
    String generatorName = DetermineGeneratorName(project, options);

    // determine output paths for solution and binary generation - they strongly depend on the platform and are independent
    Path buildSolutionOutputPath = DetermineOutputFolder(project.solutionDirectoryPath, platformType, solutionType, generatorName);
    Path buildSolutionPublishPath = DetermineOutputFolder(project.publishDirectoryPath, platformType, solutionType, generatorName);

    // create solution
    Solution sol = new Solution(solutionType, platformType, libs, project.manifestPath, buildSolutionOutputPath, buildSolutionPublishPath);

    // fill solution with content
    for (Module m : modules.modules)
      sol.scanModuleContent(m);
    sol.finalizeScannedContent();

    // resolve project and library dependencies
    // this will throw in case any dependency is mismatched
    sol.resolveDependencies(libs);

    // create generator
    GeneratedFilesCollection files = new GeneratedFilesCollection();
    SolutionSetup g = createGenerator(sol, options, generatorName);
    if (g != null) {
      g.generateFiles(files);
    } else {
      System.exit(1);
    }

    // save the output files
    {
      int numSaved = files.save(false);
      System.out.println(String.format("Saved %d files, %d were up to date out of %d total", numSaved, files.count() - numSaved, files.count()));
    }


    // print location of the license
    if (sol.generatedSolutionFile != null) {
      try {
        String relativePath = Paths.get(Utils.getCurrentJar().getPath()).getParent().relativize(sol.generatedSolutionFile).toString();

        System.out.printf("\n");
        System.out.printf("===============================================================================\n");
        System.out.printf("Solution was written to: '%s'\n", relativePath);
        System.out.printf("===============================================================================\n");
        System.out.printf("\n");

      } catch (Exception e) {

      }
    }
  }

  //---

  private static SolutionSetup createGenerator(Solution sol, KeyValueTable options, String generatorName) {
    try {
      if (generatorName.equals("vs2019")) {
        return new VsSolutionSetup(sol, options, "v142", "16.0");
      } else if (generatorName.equals("vs2017")) {
        return new VsSolutionSetup(sol, options, "v142", "15.0");
      } else if (generatorName.equals("cmake")) {
        return new CMakeSolutionSetup(sol, options);
      } else {
        System.err.printf("Unsupported solution generate '%s'\n", generatorName);
      }
    } catch (Exception e) {
      System.err.println("Error creating generator: " + e.toString());
    }

    return null;
  }

  public static String DetermineGeneratorName(ProjectManifest manifest, KeyValueTable options) {
    PlatformType platform = DeterminePlatform(options);
    String defaultGeneratorName = (platform == PlatformType.WINDOWS) ? "vs2019" : "cmake";
    return options.valueOrDefault("generator", defaultGeneratorName);
  }

  public static SolutionType DetermineSolutionType(ProjectManifest manifest, KeyValueTable options) {
    String str = options.valueOrDefault("solution", manifest.defaultSolutionType);

    Optional<SolutionType> solutionType = SolutionType.mapName(str);
    if (!solutionType.isPresent()) {
      System.err.printf("Unsupported solution type '%s', defaulting to Full Solution\n", str);
      return SolutionType.FULL;
    } else {
      return solutionType.get();
    }
  }

  public static PlatformType DeterminePlatform(KeyValueTable options) {
    PlatformType platform = PlatformType.WINDOWS;
    String platformName = options.value("platform");
    if (platformName.equals("")) {
      String OS = System.getProperty("os.name").toLowerCase();
      System.out.println("Platform configuration (-platform) not specified, trying to detect using system data (" + OS + ")");

      if (OS.indexOf("win") >= 0) {
        System.out.println("Platform detected as WINDOWS");
        platform = PlatformType.WINDOWS;
      } else if (OS.indexOf("mac") >= 0) {
        System.out.println("Platform detected as MAC");
        platform = PlatformType.MAC;
      } else if (OS.indexOf("nix") >= 0 || OS.indexOf("nux") >= 0 || OS.indexOf("aix") > 0) {
        System.out.println("Platform detected as LINUX");
        platform = PlatformType.LINUX;
      } else {
        System.err.println("Unable to detect platform. Defaulting to CMake. Good Luck!");
      }
    } else {
      java.util.Optional<PlatformType> foundPlatform = PlatformType.mapPlatform(platformName);
      if (!foundPlatform.isPresent()) {
        System.err.println("Platform '" + platformName + "' not found. Defaulting to CMake. Good Luck!");
      }
    }

    return platform;
  }
}
