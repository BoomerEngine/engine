package generators;

import project.*;
import generators.ProjectSetup;
import project.dependencies.Dependency;
import project.dependencies.DependencyType;
import utils.GeneratedFile;
import utils.GeneratedFilesCollection;
import utils.KeyValueTable;
import utils.ProjectException;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.nio.file.Files;
import java.util.ArrayList;
import java.util.Set;
import java.util.List;
import java.util.HashSet;
import java.util.Locale;
import java.util.LinkedHashSet;
import java.util.stream.Collectors;

public abstract class ProjectSourcesSetup extends ProjectSetup {

  //---

  //public ProjectSources originalSourcesProject;

  public Module module;
  public List<File> files;

  public Path sourcesAbsolutePath;

  public Path localPublicHeaderFile; // sources/../<projectname>/include/public.h" - empty if project has no public header
  public Path localPublicGlueFile; // output/generated/_shared/<projectname>.inl
  public Path localStaticInitGlueFile; // output/generated/<projectname>/static_lib_init.inl
  public Path localAutoMainFile; // output/generated/<projectname>/main.cpp
  public Path localVSIXManifestFile; // output/generated/<projectname>/source.extension.vsixmanifest

  public List<Path> localAdditionalSourceFiles = new ArrayList<>();
  public List<Path> localAdditionalHeaderFiles = new ArrayList<>();
  public List<Path> localAdditionalVSIXManifestFiles = new ArrayList<>();

  //---

  private static String SolutionPathFromGroup(Group group) {
    if (group == null) return "";
    if (group.parentGroup == null) return group.shortName;
    return SolutionPathFromGroup(group.parentGroup) + "." + group.shortName;
  }

  public ProjectSourcesSetup(ProjectSources baseSourceProject, SolutionSetup solutionSetup) {
    super(solutionSetup, baseSourceProject.shortName, baseSourceProject.mergedName, SolutionPathFromGroup(baseSourceProject.group));

    this.sourcesAbsolutePath = baseSourceProject.absolutePath;
    this.module = baseSourceProject.module;
    this.attributes = baseSourceProject.attributes;

    this.files = new ArrayList<File>();
    File precompiledHeader = null;
    for (File pf : baseSourceProject.files) {
      if (pf.shortName.endsWith("_test.cpp") && solutionSetup.solutionType == SolutionType.FINAL)
        continue;

      if (pf.platformsFilter == null || pf.platformsFilter.contains(solutionSetup.platformType)) {
        if (pf.attributes.value("pch").equals("generate")) {
          precompiledHeader = pf;
        } else {
          files.add(pf);
        }
      }
    }
    if (precompiledHeader != null)
      files.add(0, precompiledHeader);

    this.localPublicGlueFile = solutionSetup.rootGeneratedSharedPath.resolve(baseSourceProject.mergedName + "_glue.inl");
    this.localAdditionalHeaderFiles.add(this.localPublicGlueFile);

    this.localStaticInitGlueFile = this.localGeneratedPath.resolve("static_init.inl");
    this.localAdditionalHeaderFiles.add(this.localStaticInitGlueFile);

    Path publicFilePath = this.sourcesAbsolutePath.resolve("include/public.h");
    if (java.nio.file.Files.exists(publicFilePath)) {
      this.localPublicHeaderFile = module.sourceDirectory.relativize(publicFilePath);
    }

    if (attributes.hasKey("generate_main")) {
      this.localAutoMainFile = this.localGeneratedPath.resolve("main.cpp");
      this.localAdditionalSourceFiles.add(this.localAutoMainFile);
    }

    boolean hasNatVisFiles = baseSourceProject.files.stream().anyMatch(f -> f.type == FileType.NATVIS);
    if (hasNatVisFiles) {
      this.localVSIXManifestFile = this.localGeneratedPath.resolve("source.extension.vsixmanifest");
      this.localAdditionalVSIXManifestFiles.add(this.localVSIXManifestFile);
    }

    for (Dependency dep : baseSourceProject.dependencies) {
      if (null != dep.resolvedLibrary) {
        if (dep.type == DependencyType.PRIVATE_LIBRARY) {
          System.out.printf("Discovered private library '%s' used in '%s'\n", dep.resolvedLibrary.name, mergedName);
          privateLibraryDependencies.add(dep.resolvedLibrary);
        } else if (dep.type == DependencyType.PUBLIC_LIBRARY) {
          System.out.printf("Discovered public library '%s' used in '%s'\n", dep.resolvedLibrary.name, mergedName);
          publicLibraryDependencies.add(dep.resolvedLibrary);
        }
      }
    }
  }

  protected void generateWrappersForAdditionalFiles() {
    for (Path p : localAdditionalHeaderFiles)
      files.add(createFileWrapperForAutogeneratedFile(p, FileType.HEADER));
    localAdditionalHeaderFiles.clear();

    for (Path p : localAdditionalSourceFiles)
      files.add(createFileWrapperForAutogeneratedFile(p, FileType.SOURCE));
    localAdditionalSourceFiles.clear();

    for (Path p : localAdditionalVSIXManifestFiles)
      files.add(createFileWrapperForAutogeneratedFile(p, FileType.VSIXMANIFEST));
    localAdditionalVSIXManifestFiles.clear();
  }

  private File createFileWrapperForAutogeneratedFile(Path physicalPath, FileType fileType) {
    File f = File.CreateVirtualFile(physicalPath, physicalPath.getFileName(), fileType);
    f.attributes.addValue("pch", "disable");
    f.attributes.addValue("generated", "true");
    f.attributes.addValue("filter", "_auto");
    return f;
  }

  protected List<Path> extractSourceRoots() {
    // find unique root paths for all of the project that are our dependencies
    // the root paths can be generated by looking at the unique modules the projects are coming from
    Set<Path> rootPaths = new HashSet<Path>();
    for (ProjectSourcesSetup dep : collectSortedSourcesDependencies())
        rootPaths.add(dep.module.sourceDirectory);

    // than, prefer the local project src and the include directory
    rootPaths.add(sourcesAbsolutePath.resolve("src//"));
    rootPaths.add(sourcesAbsolutePath.resolve("include//"));

    // some projects may report additional source directories
    // note: the additional include directories are ALWAYS relative to the project
    List<String> additionalIncludeDirectories = attributes.values("additionalIncludeDirectory");
    if (!additionalIncludeDirectories.isEmpty()) {
      additionalIncludeDirectories.stream()
              .map(path -> sourcesAbsolutePath.resolve(path).normalize()) // resolve relative to the project directory
              .map(path -> path.toString().replace('\\','/'))
              .map(path -> Paths.get(path.endsWith("/") ? path : (path + "/")))
              .collect(Collectors.toCollection(() -> rootPaths));
    }

    // add the root directory from the module
    rootPaths.add(module.sourceDirectory);

    // finally, allows files from the shared and generated directories to be visible as well
    rootPaths.add(solutionSetup.rootGeneratedSharedPath);
    rootPaths.add(solutionSetup.rootGeneratedGluedPath);
    rootPaths.add(localGeneratedPath);

    // some projects report a "publicinclude" flag (mostly the third party libs)
    // if found, include the project include directly directly into the source roots
    // ie. make the #include "zlib.h" possible vs the #include "thirdparty/zlib/include/zlib.h"
    for (ProjectSourcesSetup dep : collectSortedSourcesDependencies()) {
      if (dep.attributes.hasKey("publicinclude")) {
        Path projectIncludePath = dep.sourcesAbsolutePath.resolve("include//");
        rootPaths.add(projectIncludePath);
      }
    }

    return new ArrayList<Path>(rootPaths);
  }

  protected boolean requiresStaticInit() {
    // maybe the project has a flag :)
    if (attributes.hasKey("noinit"))
      return false;

    // app does not require static init
    if (attributes.hasKey("app") || attributes.hasKey("console") )
      return false;

    // get all projects that are dependencies
    for (ProjectSetup dep : collectSortedDependencies())
      if (dep.mergedName.equals("base_system"))
        return true;

    // project does not depend on the "base_system", no module exists and no static initialization is required
    return mergedName.equals("base_system");
  }

  public boolean requiresRTTI() {
    // maybe the project has a flag :)
    if (attributes.hasKey("nortti"))
      return false;

    // get all projects that have the reflection in the dependencies
    for (ProjectSetup dep : collectSortedSourcesDependencies())
      if (dep.mergedName.equals("base_reflection"))
        return true;

    // project does not depend on the "base_reflection", no RTTI is needed unless its the reflection project itself
    return mergedName.equals("base_reflection");
  }

  public void generateGenericProjectFiles(GeneratedFilesCollection files) {
    // generate the _glue file first
    // this defines the project _API macros and includes public headers from other projects
    if (localPublicGlueFile != null) {
      GeneratedFile f = files.createFile(localPublicGlueFile);
      generateGenericProjectGlueFile(f);
    }

    // generate the project initialization file
    // this is used only for standalone configurations but is always generated
    if (localStaticInitGlueFile != null) {
      GeneratedFile f = files.createFile(localStaticInitGlueFile);
      generateGenericProjectStaticInitializationFile(f);
    }

    // generate the main.cpp for applications with an entry point
    if (localAutoMainFile != null) {
      GeneratedFile f = files.createFile(localAutoMainFile);
      generateGenericMainFile(f);
    }

    // generate the VSIX manifest for NatVis files
    if (localVSIXManifestFile != null) {
      GeneratedFile f = files.createFile(localVSIXManifestFile);
      //generateVSIXManifestFile(f);
    }

    // bison files
    this.files.stream()
            .filter(f -> f.type == FileType.BISON)
            .forEach(f -> generateBisonOutputs(f));

    // MOC files for QT
    this.files.stream()
            .filter(f -> f.type == FileType.HEADER)
            .filter(f -> f.attributes.hasKey("qt_moc"))
            .forEach(f -> generateQTMocFile(f));

    // QRC files for QT
    this.files.stream()
            .filter(f -> f.type == FileType.QTRESOURCES)
            .forEach(f -> generateQTResourceFile(f));

    // UI files for QT
    this.files.stream()
            .filter(f -> f.type == FileType.QTUI)
            .forEach(f -> generateQTDesignedFile(f));

    // inject extra files into file list
    generateWrappersForAdditionalFiles();
  }



  //----

  private void generateGenericProjectStaticInitializationFile(GeneratedFile f) {
    f.writeln("/***");
    f.writeln("* Boomer Engine Static Lib Initialization Code");
    f.writeln("* Auto generated, do not modify");
    f.writeln("* Build system source code licensed under MIP license");
    f.writeln("***/");
    f.writeln( "");

    // determine if project requires static initialization (the apps and console apps require that)
    // then pull in the library linkage, for apps we pull much more crap
    boolean hasStaticInitialization = attributes.hasKey("app") || attributes.hasKey("console");
    generateProjectLibrariesLinkage(hasStaticInitialization, f);

    // static initialization part is only generated for apps
    if (hasStaticInitialization) {
      f.writeln( "void InitializeStaticDependencies()");
      f.writeln( "{");

      List<ProjectSourcesSetup> dependencies = collectSortedSourcesDependencies();
      if (!dependencies.isEmpty()) {
        f.writeln( "#ifdef BUILD_AS_LIBS");

        // if we build based on static libraries we need to "touch" the initialization code from other modules
        for (ProjectSourcesSetup dep : dependencies) {
          if (dep.requiresStaticInit()) {
            f.writelnf("    extern void InitModule_%s();", dep.mergedName);
            f.writelnf("    InitModule_%s();", dep.mergedName);
          }
        };

        f.writeln( "#else");

        // if we are build based on dynamic libs (dlls)
        // make sure that they are loaded on time
        // THIS IS TEMPORARY UNTIL WE DO A PROPER CLASS LOADER
        for (ProjectSourcesSetup dep : dependencies) {
          //System.out.printf("Adding DLL load '%s' on '%s' (%s)\n", mergedName, dep.mergedName, dep.attributes.toString());
          if (!dep.attributes.hasKey("app") && !dep.attributes.hasKey("console") )
            f.writelnf( "    base::modules::LoadDynamicModule(\"%s\");", dep.mergedName);
        };

        f.writeln( "#endif");
      }

      // initialize ourselves
      f.writelnf( "    extern void InitModule_%s();", mergedName);
      f.writelnf( "    InitModule_%s();", mergedName);

      f.writeln( "}");
    }
  }

  protected abstract void generateProjectLibrariesLinkage(boolean hasStaticInitialization, GeneratedFile f);

  //----

  private void generateGenericProjectGlueFile(GeneratedFile f) {
    String macroName = mergedName.toUpperCase() + "_GLUE";
    String apiName = mergedName.toUpperCase() + "_API";
    String exportsMacroName = mergedName.toUpperCase() + "_EXPORTS";

    f.writeln("/***");
    f.writeln("* Boomer Engine Glue Code");
    f.writeln("* Build system source code licensed under MIP license");
    f.writeln("* Auto generated, do not modify");
    f.writeln("***/");
    f.writeln("");    f.writeln("");
    f.writelnf( "#ifndef " + macroName);
    f.writelnf( "#define " + macroName);
    f.writeln( "");

    // Interface macro
    f.writeln( "#ifdef BUILD_AS_LIBS");
    f.writeln( "  #define " + apiName);
    f.writeln( "#else");
    f.writeln( "  #ifdef " + exportsMacroName);
    f.writelnf( "    #define %s __declspec( dllexport )", apiName);
    f.writeln( "  #else");
    f.writelnf( "    #define %s __declspec( dllimport )", apiName);
    f.writeln( "  #endif");
    f.writeln( "#endif");

    f.writeln( "");
    f.writeln( "#endif");

    // get the dependencies
    List<ProjectSourcesSetup> dependencies = collectSortedSourcesDependencies();
    if (!dependencies.isEmpty()) {

      f.writeln("");
      f.writeln("// Public header from project dependencies:");

      for (ProjectSourcesSetup p : dependencies) {
        if (p.localPublicHeaderFile != null) {
          f.writelnf("#include \"%s\"", p.localPublicHeaderFile);
        }
      }
    }
  }

  private void generateGenericMainFile(GeneratedFile f) {

    boolean isConsole = attributes.hasKey("console");
    boolean isEditor = attributes.hasKey("editor");
    boolean noApp = attributes.hasKey("noapp");

    f.writeln("/***");
    f.writeln("* Boomer Engine Entry Point");
    f.writeln("* Auto generated, do not modify");
    f.writeln("* Build system source code licensed under MIP license");
    f.writeln("*");
    f.writeln("* [# filter: _auto #]");
    f.writeln("* [# pch: disabled #]");
    f.writeln("***/");

    f.writeln("");
    f.writeln("#include \"build.h\"");
    f.writeln("#include \"static_init.inl\"");
    f.writeln("#include \"base/app/include/launcherMacros.h\"");
    f.writeln("#include \"base/app/include/commandline.h\"");
    f.writeln("#include \"base/app/include/launcherPlatform.h\"");
    f.writeln("#include \"base/app/include/application.h\"");

    f.writeln("");
    f.writeln("void* GCurrentModuleHandle = nullptr;");
    f.writeln("");

    if (!noApp) {
      f.writeln("");
      f.writeln("extern base::app::IApplication& GetApplicationInstance();");
      f.writeln("");
    }

    f.writeln("#define MACRO_TXT2(x) #x");
    f.writeln("#define MACRO_TXT(x) MACRO_TXT2(x)");
    f.writeln("");

    f.writeln("LAUNCHER_MAIN()");
    f.writeln("{");
    f.writeln("      // initialize all static modules used by this application");
    f.writeln("      GCurrentModuleHandle = LAUNCHER_APP_HANDLE();");
    f.writeln("      InitializeStaticDependencies();");
    f.writeln("");
    f.writeln("      // parse system command line");
    f.writeln("      base::app::CommandLine cmdline;");
    f.writeln("      if (!LAUNCHER_PARSE_CMDLINE())");
    f.writeln("        return -1;");
    f.writeln("");

    if (noApp)
      f.writeln("      cmdline.param(\"noapp\", nullptr);");

    if (isConsole)
      f.writeln("      cmdline.param(\"console\", nullptr);");

    if (isEditor)
      f.writeln("      cmdline.param(\"editor\", nullptr);");

    if (attributes.hasKey("noscripts"))
      f.writeln("      cmdline.param(\"noscripts\", nullptr);");

    //f.writeln("      #ifdef MODULE_NAME");
    //f.writeln("        cmdline.addParam(\"moduleName\", MACRO_TXT(MODULE_NAME));");
    //f.writeln("      #endif");
    //f.writeln("");
    //f.writeln("      auto& app = base::platform::GetLaunchPlatform();");
    //f.writeln("      return app.run(cmdline);");

    f.writeln("      // initialize app");
    f.writeln("      auto& platform = base::platform::GetLaunchPlatform();");
    if (noApp)
      f.writeln("      if (!platform.platformStart(cmdline, nullptr))");
    else
      f.writeln("      if (!platform.platformStart(cmdline, &GetApplicationInstance()))");
    f.writeln("        return -1; // failed to initialize");
    f.writeln("");

    {
      f.writeln("      // main loop");
      f.writeln("      while (platform.platformUpdate()) { platform.platformIdle(); };");
      f.writeln("");
    }

    f.writeln("      // cleanup");
    f.writeln("      cmdline = base::app::CommandLine(); // prevent leaks past app.cleanup()");
    f.writeln("      platform.platformCleanup();");

    f.writeln("      return 0;");
    f.writeln("    }");
  }
  private void generateBisonOutputs(File sourcefile) {
    // get the name of the reflection file
    Path parserFile = localGeneratedPath.resolve(sourcefile.coreName + "_Parser.cpp");
    Path symbolsFile = localGeneratedPath.resolve(sourcefile.coreName + "_Symbols.h");
    Path reportPath = localGeneratedPath.resolve(sourcefile.coreName + "_Report.txt");

    // run the bison
    try {
      Path bisonPath = null;
      Path envPath = null;

      String OS = System.getProperty("os.name", "generic").toLowerCase(Locale.ENGLISH);
      if (OS.indexOf("win") >= 0) {
        bisonPath = solutionSetup.libs.resolveFileInLibraries("bison/win/bin/win_bison.exe");
        envPath = solutionSetup.libs.resolveFileInLibraries("bison/win/share/bison");
      } else {
        bisonPath = solutionSetup.libs.resolveFileInLibraries("bison/linux/bin/bison");
        envPath = solutionSetup.libs.resolveFileInLibraries("bison/linux/share/bison");
      }

      if (!Files.exists(bisonPath)) {
        System.err.printf("Unable to find Bison executable at %s, Bison files will NOT be build, projects may not compile\n", bisonPath.toString());
        return;
      }

      Files.createDirectories(parserFile.getParent());
      Files.createDirectories(symbolsFile.getParent());
      Files.createDirectories(reportPath.getParent());

      if (GeneratedFile.ShouldGenerateFile(sourcefile.absolutePath, parserFile)) {
        String commandLine = String.format("%s %s -o%s --defines=%s --verbose --report-file=%s", bisonPath.toString(), sourcefile.absolutePath.toString(), parserFile.toString(), symbolsFile.toString(), reportPath.toString());
        System.out.println(String.format("Running command '%s' with env '%s'", commandLine, envPath));
        Process p = Runtime.getRuntime().exec(commandLine, new String[]{"BISON_PKGDATADIR=" + envPath.toString()});
        p.waitFor();

        // forward output
        BufferedReader stdOut = new BufferedReader(new InputStreamReader(p.getInputStream()));
        String s = null;
        while ((s = stdOut.readLine()) != null) {
          System.out.println(s);
        }

        // forward errors
        BufferedReader stdErr = new BufferedReader(new InputStreamReader(p.getErrorStream()));
        while ((s = stdErr.readLine()) != null) {
          System.err.println(s);
        }

        if (0 != p.exitValue())
          throw new ProjectException("Unable to process BISON file " + sourcefile.shortName + ", failed with error code: " + p.exitValue());

        // tag the file with the same timestamp as the source file
        // Files.setLastModifiedTime(parserFile, Files.getLastModifiedTime(sourcefile.absolutePath));
        //Files.setLastModifiedTime(symbolsFile, Files.getLastModifiedTime(sourcefile.absolutePath));
      }
    } catch (Exception e) {
      e.printStackTrace();
      throw new ProjectException("Unable to start BISON", e);
    }

    // remember about the file
    localAdditionalSourceFiles.add(parserFile);
    localAdditionalHeaderFiles.add(symbolsFile);
  }

  private void generateQTMocFile(File sourcefile) {
    // get the name of the reflection file
    Path mocFile = localGeneratedPath.resolve(sourcefile.coreName + "_moc.cpp");
    Path buildFile = sourcesAbsolutePath.resolve("src/build.h");

    // run the MOC
    try {
      Path mocPath = null;

      String qt5Path = System.getenv("QT5_DIR");

      if (qt5Path != null) {
        String OS = System.getProperty("os.name", "generic").toLowerCase(Locale.ENGLISH);
        if (OS.indexOf("win") >= 0) {
          if (!qt5Path.endsWith("\\")) qt5Path += "\\";
          mocPath = solutionSetup.libs.resolveFileInLibraries(String.format("%sbin/moc.exe", qt5Path));
        } else {
          if (!qt5Path.endsWith("/")) qt5Path += "/";
          mocPath = solutionSetup.libs.resolveFileInLibraries(String.format("%sbin/moc", qt5Path));
        }

        Files.createDirectories(mocFile.getParent());

        if (GeneratedFile.ShouldGenerateFile(sourcefile.absolutePath, mocFile)) {
          String commandLine = String.format("%s -o%s -b%s %s", mocPath.toString(), mocFile.toString(), buildFile.toString(), sourcefile.absolutePath.toString());
          System.out.println(String.format("Generating MOC file for '%s'", sourcefile.absolutePath));
          Process p = Runtime.getRuntime().exec(commandLine, null);
          p.waitFor();

          // forward output
          BufferedReader stdOut = new BufferedReader(new InputStreamReader(p.getInputStream()));
          String s = null;
          while ((s = stdOut.readLine()) != null) {
            System.out.println(s);
          }

          // forward errors
          BufferedReader stdErr = new BufferedReader(new InputStreamReader(p.getErrorStream()));
          while ((s = stdErr.readLine()) != null) {
            System.err.println(s);
          }

          if (0 != p.exitValue())
            throw new ProjectException("Unable to process MOC file " + sourcefile.shortName + ", failed with error code: " + p.exitValue());

          // tag the file with the same timestamp as the source file
          //Files.setLastModifiedTime(mocFile, Files.getLastModifiedTime(sourcefile.absolutePath));
        } else {
          System.out.println(String.format("Skipped generating MOC file for '%s' - up to date", sourcefile.absolutePath));
        }

        localAdditionalSourceFiles.add(mocFile);
      }
    } catch (Exception e) {
      e.printStackTrace();
      throw new ProjectException("Unable to start MOC", e);
    }
  }

  private void generateQTResourceFile(File sourcefile) {
    // get the name of the resource file
    Path qrcFile = localGeneratedPath.resolve(sourcefile.coreName + "_qrc.cpp");

    // run the RCC
    try {
      Path toolPath = null;

      String qt5Path = System.getenv("QT5_DIR");

      if (qt5Path != null) {
        String OS = System.getProperty("os.name", "generic").toLowerCase(Locale.ENGLISH);
        if (OS.indexOf("win") >= 0) {
          if (!qt5Path.endsWith("\\")) qt5Path += "\\";
          toolPath = solutionSetup.libs.resolveFileInLibraries(String.format("%sbin/rcc.exe", qt5Path));
        } else {
          if (!qt5Path.endsWith("/")) qt5Path += "/";
          toolPath = solutionSetup.libs.resolveFileInLibraries(String.format("%sbin/rcc", qt5Path));
        }

        Files.createDirectories(qrcFile.getParent());

        if (GeneratedFile.ShouldGenerateFile(sourcefile.absolutePath, qrcFile)) {
          String commandLine = String.format("%s --output %s --compress 9 --name %s %s", toolPath.toString(), qrcFile.toString(), sourcefile.coreName, sourcefile.absolutePath.toString());
          System.out.println(String.format("Processing QRC file '%s'", sourcefile.absolutePath));
          Process p = Runtime.getRuntime().exec(commandLine, null);
          p.waitFor();

          // forward output
          BufferedReader stdOut = new BufferedReader(new InputStreamReader(p.getInputStream()));
          String s = null;
          while ((s = stdOut.readLine()) != null) {
            System.out.println(s);
          }

          // forward errors
          BufferedReader stdErr = new BufferedReader(new InputStreamReader(p.getErrorStream()));
          while ((s = stdErr.readLine()) != null) {
            System.err.println(s);
          }

          if (0 != p.exitValue())
            throw new ProjectException("Unable to process QRC file " + sourcefile.shortName + ", failed with error code: " + p.exitValue());

          // tag the file with the same timestamp as the source file
          //Files.setLastModifiedTime(qrcFile, Files.getLastModifiedTime(sourcefile.absolutePath));
        } else {
          System.out.println(String.format("Skipped processing QRC file '%s' - up to date", sourcefile.absolutePath));
        }

        localAdditionalSourceFiles.add(qrcFile);
      }
    } catch (Exception e) {
      e.printStackTrace();
      throw new ProjectException("Unable to start RCC", e);
    }
  }

  private void generateQTDesignedFile(File sourcefile) {
    // get the name of the ui file
    Path uiFile = localGeneratedPath.resolve(sourcefile.coreName + ".ui.h");

    // run the RCC
    try {
      Path toolPath = null;

      String qt5Path = System.getenv("QT5_DIR");

      if (qt5Path != null) {
        String OS = System.getProperty("os.name", "generic").toLowerCase(Locale.ENGLISH);
        if (OS.indexOf("win") >= 0) {
          if (!qt5Path.endsWith("\\")) qt5Path += "\\";
          toolPath = solutionSetup.libs.resolveFileInLibraries(String.format("%sbin/uic.exe", qt5Path));
        } else {
          if (!qt5Path.endsWith("/")) qt5Path += "/";
          toolPath = solutionSetup.libs.resolveFileInLibraries(String.format("%sbin/uic", qt5Path));
        }

        Files.createDirectories(uiFile.getParent());

        if (GeneratedFile.ShouldGenerateFile(sourcefile.absolutePath, uiFile)) {
          String commandLine = String.format("%s --no-protection --postfix _%s --output %s %s", toolPath.toString(), sourcefile.coreName, uiFile.toString(), sourcefile.absolutePath.toString());
          System.out.println(String.format("Processing UI file '%s'", sourcefile.absolutePath));
          Process p = Runtime.getRuntime().exec(commandLine, null);
          p.waitFor();

          // forward output
          BufferedReader stdOut = new BufferedReader(new InputStreamReader(p.getInputStream()));
          String s = null;
          while ((s = stdOut.readLine()) != null) {
            System.out.println(s);
          }

          // forward errors
          BufferedReader stdErr = new BufferedReader(new InputStreamReader(p.getErrorStream()));
          while ((s = stdErr.readLine()) != null) {
            System.err.println(s);
          }

          if (0 != p.exitValue())
            throw new ProjectException("Unable to process QRC file " + sourcefile.shortName + ", failed with error code: " + p.exitValue());

          // tag the file with the same timestamp as the source file
          //Files.setLastModifiedTime(uiFile, Files.getLastModifiedTime(sourcefile.absolutePath));
        } else {
          System.out.println(String.format("Skipped processing UI file '%s' - up to date", sourcefile.absolutePath));
        }

        localAdditionalHeaderFiles.add(uiFile);
      }
    } catch (Exception e) {
      e.printStackTrace();
      throw new ProjectException("Unable to start UIC", e);
    }
  }

  //---

}
