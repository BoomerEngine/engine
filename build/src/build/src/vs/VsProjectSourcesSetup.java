package vs;

import com.google.common.collect.Sets;
import generators.PlatformType;
import generators.ProjectSetup;
import generators.ProjectSourcesSetup;
import generators.SolutionType;
import library.Config;
import project.File;
import project.FileType;
import project.ProjectSources;
import library.Library;
import project.dependencies.Dependency;
import project.dependencies.DependencyType;
import utils.GeneratedFile;
import utils.GeneratedFilesCollection;
import utils.Utils;

import java.io.IOException;
import java.util.ArrayList;
import java.util.List;
import java.util.Set;
import java.util.LinkedHashSet;
import java.util.stream.Stream;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.nio.file.Files;
import java.nio.file.StandardCopyOption;
import java.util.Comparator;
import java.util.stream.Collectors;


public class VsProjectSourcesSetup extends ProjectSourcesSetup {

  public Path localProjectFilePath; // output/projects/<projectname>/<projectname>.vcxproj
  public Path localFiltersFilePath; // output/projects/<projectname>/<projectname>.vcxproj.filters

  public String toolset;
  public String projectVersion;

  public VsProjectSourcesSetup(ProjectSources baseProject, VsSolutionSetup solutionSetup) {
    super(baseProject, solutionSetup);

    this.toolset = solutionSetup.toolset;
    this.projectVersion = solutionSetup.projectVersion;
    this.localProjectFilePath = this.localOutputPath.resolve(mergedName + ".vcxproj");
    this.localFiltersFilePath = this.localOutputPath.resolve(mergedName + ".vcxproj.filters");

    this.generatedProjectPath = this.localProjectFilePath;

    if (baseProject.group != null) {
      if (baseProject.mergedName.startsWith("examples_")) {
        solutionPath = "Examples";
      } else {
        solutionPath = "BoomerEngine." + solutionPath;
      }
    }
  }

  private void generateProjectDependencyEntries(GeneratedFile f) {
    for (ProjectSetup p : collectSortedDependencies()) {
      if (p.generatedProjectPath != null) {
        f.writelnf(" <ProjectReference Include=\"%s\">", p.generatedProjectPath);
        f.writelnf("   <Project>%s</Project>", Utils.guidFromText("project_" + p.mergedName));
        f.writeln(" </ProjectReference>");
      }
    }
  }

  @Override
  public void generateFiles(GeneratedFilesCollection files) {
    generateGenericProjectFiles(files);
    generateProjectMainFile(files);
    generateProjectFiltersFile(files);
  }

  private void generateVSIXManifestFile(GeneratedFile f) {
    f.writeln("<?xml version=\"1.0\" encoding=\"utf-8\"?>");
    f.writeln("<PackageManifest Version=\"2.0.0\" xmlns=\"http://schemas.microsoft.com/developer/vsx-schema/2011\" xmlns:d=\"http://schemas.microsoft.com/developer/vsx-schema-design/2011\">");
    f.writeln("<Metadata>");
    f.writeln("<Identity Id=\"NatvisSample..e72aa83c-bb6f-4910-8e9f-793052cd112a\" Version=\"1.0\" Language=\"en-US\" Publisher=\"MS\" />");
    f.writeln("<DisplayName>NatvisSample</DisplayName>");
    f.writeln("<Description>Sample project that demonstrates how to package type visualizers in a VSIX package</Description>");
    f.writeln("</Metadata>");
    f.writeln("<Installation>");
    f.writeln("<InstallationTarget Id=\"Microsoft.VisualStudio.Pro\" Version=\"11.0\" />");
    f.writeln("</Installation>");
    f.writeln("<Dependencies>");
    f.writeln("<Dependency Id=\"Microsoft.Framework.NDP\" DisplayName=\"Microsoft .NET Framework\" d:Source=\"Manual\" Version=\"4.5\"/>");
    f.writeln("</Dependencies>");
    f.writeln("<Assets>");

    this.files.stream()
            .filter(pf -> pf.type == FileType.NATVIS)
            .forEach(pf -> f.writelnf("<Asset Type=\"NativeVisualizer\" Path=\"%s\"/>", pf.absolutePath));

    f.writeln("</Assets>");
    f.writeln("</PackageManifest>");
  }

  private void generateProjectMainFile(GeneratedFilesCollection files) {
    GeneratedFile f = files.createFile(localProjectFilePath);

    f.writeln("<?xml version=\"1.0\" encoding=\"utf-8\"?>");
    f.writeln("<!-- Auto generated file, please do not edit -->");

    f.writelnf("<Project DefaultTargets=\"Build\" ToolsVersion=\"%s\" xmlns=\"http://schemas.microsoft.com/developer/msbuild/2003\">", projectVersion);

    f.writelnf("<Import Project=\"%s\\SharedConfigurationSetup.props\"/>", solutionSetup.buildScriptsPath);

    f.writeln("<PropertyGroup>");
    f.writelnf("  <PlatformToolset>%s</PlatformToolset>", toolset);
    f.writelnf("  <VCProjectVersion>%s</VCProjectVersion>", projectVersion);
    f.writeln("  <WindowsTargetPlatformVersion>10.0</WindowsTargetPlatformVersion>");

    List<Path> sourceRoots = extractSourceRoots();
    sourceRoots.forEach( root -> f.writelnf(" 	<SourcesRoot>$(SourcesRoot);%s\\</SourcesRoot>", root));

    generateProjectLibrariesIncludes(f);

    Path relativeProjectPath = module.sourceDirectory.relativize(sourcesAbsolutePath);

    f.writelnf(" 	<ProjectOutputPath>%s\\</ProjectOutputPath>", localTempPath);
    f.writelnf(" 	<ProjectToolsPath>%s\\</ProjectToolsPath>", solutionSetup.buildToolsPath);
    f.writelnf(" 	<ProjectGeneratedPath>%s\\</ProjectGeneratedPath>", localGeneratedPath);
    f.writelnf(" 	<ProjectPublishPath>%s\\</ProjectPublishPath>", solutionSetup.rootPublishPath);
    f.writelnf(" 	<ProjectSourceRoot>%s\\</ProjectSourceRoot>", sourcesAbsolutePath);
    //f.writelnf(" 	<ProjectModuleName>%s</ProjectModuleName>", originalmodule.getName());
    f.writelnf(" 	<ProjectPathName>%s</ProjectPathName>", relativeProjectPath.toString());

    if (requiresRTTI())
      f.writelnf(" 	<ProjectRequiresRTTI>1</ProjectRequiresRTTI>");

    if (localPublicGlueFile != null) {
      f.writelnf(" 	<ProjectGlueFile>%s</ProjectGlueFile>", localPublicGlueFile);
    }

    if (localStaticInitGlueFile != null) {
      f.writelnf(" 	<ProjectStaticInitFile>%s</ProjectStaticInitFile>", localStaticInitGlueFile);
    }

    if (localPublicHeaderFile != null) {
      f.writelnf(" 	<ProjectPublicHeaderFile>%s</ProjectPublicHeaderFile>", localPublicHeaderFile);
    }

    if (attributes.hasKey("warn3")) {
      f.writeln("     <ProjectWarningLevel>Level3</ProjectWarningLevel>");
    }

    if (attributes.hasKey("app")) {
      f.writeln(" 	<ModuleType>App</ModuleType>");
    } else if (attributes.hasKey("console")) {
      f.writeln(" 	<ModuleType>ConsoleApp</ModuleType>");
    } else {
      f.writeln(" 	<ModuleType>Lib</ModuleType>");
    }

    if (solutionSetup.solutionType == SolutionType.FINAL)
      f.writeln(" 	<SolutionType>Player</SolutionType>");
    else
      f.writeln(" 	<SolutionType>Dev</SolutionType>");

    f.writelnf(" 	<ProjectPreprocessorDefines>%s_EXPORTS;PROJECT_NAME=%s;$(ProjectPreprocessorDefines)</ProjectPreprocessorDefines>", mergedName.toUpperCase(), mergedName);

    String perProjectDefines = "";
    for (ProjectSourcesSetup ps : collectSortedSourcesDependencies()) {
      if (!perProjectDefines.isEmpty()) perProjectDefines += ";";
      perProjectDefines += String.format("HAS_%s", ps.mergedName.toUpperCase());
    }
    if (!perProjectDefines.isEmpty())
      f.writelnf(" 	<ProjectPreprocessorDefines>%s;$(ProjectPreprocessorDefines)</ProjectPreprocessorDefines>", perProjectDefines);

    if (attributes.hasKey("console")) {
      f.writeln(" 	<ProjectPreprocessorDefines>CONSOLE;$(ProjectPreprocessorDefines)</ProjectPreprocessorDefines>");
    }

    f.writelnf(" 	<ProjectGuid>%s</ProjectGuid>", Utils.guidFromText("project_" + mergedName));
    f.writelnf(" 	<RootNamespace></RootNamespace>", mergedName);
    f.writeln("</PropertyGroup>");

    f.writelnf("<Import Project=\"%s\\SharedItemGroups.props\"/>", solutionSetup.buildScriptsPath);

    long numAssemblyFiles = this.files.stream().filter(pf -> pf.type == FileType.ASSEMBLY).count();
    if (numAssemblyFiles > 0) {
      f.writeln("<ImportGroup Label=\"ExtensionSettings\" >");
      f.writeln("  <Import Project=\"$(VCTargetsPath)\\BuildCustomizations\\masm.props\" />");
      f.writeln("</ImportGroup>");
    }

    f.writeln("<ItemGroup>");
    for (File pf : this.files)
      generateProjectFileEntry(pf, f);
    f.writeln("</ItemGroup>");

    f.writeln("<ItemGroup>");
    generateProjectDependencyEntries(f);
    f.writeln("</ItemGroup>");

    f.writelnf(" <Import Project=\"%s\\Shared.targets\"/>", solutionSetup.buildScriptsPath);
    f.writeln(" <Import Project=\"$(VCTargetsPath)\\Microsoft.Cpp.targets\"/>");

    if (numAssemblyFiles > 0) {
      f.writeln("<ImportGroup Label=\"ExtensionTargets\">");
      f.writeln("  <Import Project=\"$(VCTargetsPath)\\BuildCustomizations\\masm.targets\" />");
      f.writeln("</ImportGroup>");
    }

    f.writeln("</Project>");
  }


  private static Stream<Path> decompilePathIntoSubPaths(Path path) {
    if (path == null) {
      return Stream.empty();
    } else {
      return Stream.concat(decompilePathIntoSubPaths(path.getParent()), Stream.of(path));
    }
  }

  private List<Path> generateUniqueFilterPaths(List<File> files)
  {
    // get all (unique) filter paths nicely in a set
    return new ArrayList<>(files.stream()
            .map(file -> file.attributes.value("filter")) // we are only interested in files with the filter attribute
            .map(attr -> Paths.get(attr)) // convert the string to a relative path
            .flatMap(path -> decompilePathIntoSubPaths(path)) // convert a single path into pieces
            .sorted(Comparator.comparingInt(a -> a.getNameCount())) // sort all path pieces so the smaller paths comes first
            .collect(Collectors.toCollection(() -> new LinkedHashSet<>())));
  }

  private void generateProjectFiltersFile(GeneratedFilesCollection files) {
    GeneratedFile f = files.createFile(localFiltersFilePath);

    f.writeln("<?xml version=\"1.0\" encoding=\"utf-8\"?>");
    f.writeln("<!-- Auto generated file, please do not edit -->");
    f.writeln("<Project ToolsVersion=\"4.0\" xmlns=\"http://schemas.microsoft.com/developer/msbuild/2003\">");

    // files section
    {
      f.writeln("<ItemGroup>");
      for (File pf : this.files)
        generateProjectFiltersFileEntry(pf, f);
      f.writeln("</ItemGroup>");
    }

    // filter section
    {
      f.writeln("<ItemGroup>");

      List<Path> filterPaths = generateUniqueFilterPaths(this.files);
      for (Path filter : filterPaths) {
        String txt = filter.toString();
        if (!txt.isEmpty()) {
          String guid = utils.Utils.guidFromText("filter" + txt);
          f.writelnf("  <Filter Include=\"%s\">", txt);
          f.writelnf("    <UniqueIdentifier>%s</UniqueIdentifier>", guid);
          f.writelnf("  </Filter>");
        }
      }

      f.writeln("</ItemGroup>");
    }

    f.writeln("</Project>");
  }

  private void generateProjectFileEntry(File pf, GeneratedFile f) {
    switch (pf.type) {

      case SOURCE: {
        f.writelnf("   <ClCompile Include=\"%s\">", pf.absolutePath);

        String pch = pf.attributes.value("pch");
        if (pch.equals("generate")) {
          f.writeln("      <PrecompiledHeader>Create</PrecompiledHeader>");
        } else if (pch.equals("disable") || pch.equals("disabled")) {
          f.writeln("      <PrecompiledHeader>NotUsing</PrecompiledHeader>");
        }

        f.writeln("   </ClCompile>");
        break;
      }

      case HEADER: {
        f.writelnf("   <ClInclude Include=\"%s\"/>", pf.absolutePath);
        break;
      }

      case ASSEMBLY: {
        f.writelnf("   <MASM Include=\"%s\"/>", pf.absolutePath);
        break;
      }

      case RESOURCE: {
        f.writelnf("   <ResourceCompile Include=\"%s\"/>", pf.absolutePath);
        break;
      }

      case BISON: {
        f.writelnf("   <None Include=\"%s\">", pf.absolutePath);
        f.writelnf("      <SubType>Bison</SubType>");
        /*Optional<String> prefix = pf.attributes.valueSafe("bison_prefix");
        if (prefix.isPresent()) {
          f.writelnf("     <AdditionalArguments>-p %s</AdditionalArguments>", prefix.get());
        }*/

        f.writeln("   </None>");
        break;
      }

      case PROTO: {
        f.writelnf("   <ProtoFile Include=\"%s\"/>", pf.absolutePath);
        break;
      }

      case FLEX: {
        f.writelnf("   <FlexFile Include=\"%s\"/>", pf.absolutePath);
        break;
      }

      case ANTLR: {
        f.writelnf("   <AntlrFile Include=\"%s\"/>", pf.absolutePath);
        break;
      }

      case CONFIG: {
        f.writelnf("   <ConfigFile Include=\"%s\"/>", pf.absolutePath);
        break;
      }

      case NATVIS: {
        f.writelnf("   <Content Include=\"%s\">", pf.absolutePath);
        f.writelnf("      <IncludeInVSIX>true</IncludeInVSIX>");
        f.writelnf("   </Content>");
        break;
      }

      case VSIXMANIFEST:
      {
        f.writelnf("   <None Include=\"%s\">", pf.absolutePath);
        f.writelnf("      <SubType>Designer</SubType>");
        f.writelnf("   </None>");
        break;
      }
    }
  }

  private void generateProjectFiltersFileEntry(File pf, GeneratedFile f) {
    String filterType = "";

    switch (pf.type) {

      case SOURCE: {
        filterType = "ClCompile";
        break;
      }

      case ASSEMBLY: {
        filterType = "MASM";
        break;
      }

      case HEADER: {
        filterType = "ClInclude";
        break;
      }

      case RESOURCE: {
        filterType = "ResourceCompile";
        break;
      }

      case BISON: {
        filterType = "None";
        break;
      }

      case PROTO: {
        filterType = "ProtoFile";
        break;
      }

      case FLEX: {
        filterType = "FlexFile";
        break;
      }

      case ANTLR: {
        filterType = "AntlrFile";
        break;
      }

      case CONFIG: {
        filterType = "ConfigFile";
        break;
      }

      case NATVIS: {
        filterType = "Content";
        break;
      }

      case VSIXMANIFEST: {
        filterType = "None";
        break;
      }
    }

    f.writelnf("  <%s Include=\"%s\">", filterType, pf.absolutePath);

    String filter = pf.attributes.value("filter");
    if (!filter.equals("")) {
      String betterFilter = filter.replace('/', '\\');
      f.writelnf("    <Filter>%s</Filter>", betterFilter);
    }

    f.writelnf("  </%s>", filterType);
  }

  @Override
  protected void generateProjectLibrariesLinkage(boolean hasStaticInitialization, GeneratedFile f) {
    // get all public and internal libraries that this project is using
    // NOTE: this does not include private libraries from another projects (they are needed only if we build something that must be self-contained)
    List<Library> libs = collectLibraries("internal");
    if (!libs.isEmpty()) {
      f.writeln("#ifndef BUILD_AS_LIBS");

      // generate the libraries linkage
      // this adds the required #pragma comment lib
      generateProjectLibrariesLinkage(libs, f);

      f.writeln("#endif");
      f.writeln("");
    }

    // app only linkage
    if (hasStaticInitialization)
    {
      // we are trying to build self-contained module, need to initialize the private libraries
      // from all our dependencies as well
      List<Library> allLibs = collectLibraries("all");
      allLibs.removeAll(libs);

      if (!allLibs.isEmpty()) {
        f.writeln("#ifdef BUILD_AS_LIBS");

        // do not link libs that were already linked
        generateProjectLibrariesLinkage(allLibs, f);

        f.writeln("#endif");
        f.writeln("");
      }
    }
  }

  private void generateProjectLibrariesLinkage(List<Library> libs, GeneratedFile f) {
    for (String config : solutionSetup.allConfigs)
      for (String platform  : solutionSetup.allPlatforms)
        for (Library l : libs)
          generateProjectLibraryLinkage(l, platform, config, f);
  }

  private void generateProjectLibraryLinkage(Library lib, String platformName, String configName, GeneratedFile f) {
    // get the library setup for given platform and config
    Config.MergedState setup = lib.getSetup(platformName, configName);

    // do the #pragma lib for all the library paths
    // NOTE: in normal makefile we would have to end until the link phase with this
    if (!setup.getLibPaths().isEmpty())
    {
      f.writelnf("// linkage from library %s", lib.name);
      f.writelnf("#if defined(BUILD_%s) && defined(PLATFORM_%s)", configName.toUpperCase(), platformName.toUpperCase());

      for (Path path : setup.getLibPaths())
        f.writelnf("    #pragma comment( lib, \"%s\")", path.toString().replaceAll("\\\\", "\\\\\\\\"));

      f.writelnf("#endif");
    }
  }

  private void generateProjectLibrariesIncludes(GeneratedFile f) {
    VsSolutionSetup solution = (VsSolutionSetup)solutionSetup;

    // for every platform/config generate the list of libraries to include
    for (String configName : solutionSetup.allConfigs) {
      for (String platformName : solutionSetup.allPlatforms) {
        for (Library lib : collectLibraries("internal")) {

          // get the library setup for given platform and config
          Config.MergedState setup = lib.getSetup(platformName, configName);

          // emit the library include paths into the project
          for (Path path : setup.getIncludePaths()) {
             f.writelnf("	<LibraryIncludePath Condition=\"'$(Platform)' == '%s' and '$(Configuration)'=='%s'\">$(LibraryIncludePath);%s\\</LibraryIncludePath>", platformName, configName, path);
          }

          // copy files
          for (Config.DeployFile path : setup.getDeployFiles()) {
            if (Files.exists(path.sourcePath)) {
              Path publishPath = solutionSetup.rootPublishPath.resolve(String.format("%s", configName.toLowerCase()));
              Path targetPath = publishPath.resolve(path.targetPath);
              if (GeneratedFile.ShouldCopyFile(path.sourcePath, targetPath)) {
                System.out.printf("Copying file \"%s\" to \"%s\"\n", path.sourcePath, targetPath);
                try {
                  Files.createDirectories(targetPath.getParent());
                  Files.copy(path.sourcePath, targetPath, StandardCopyOption.REPLACE_EXISTING);
                  Files.setLastModifiedTime(targetPath, Files.getLastModifiedTime(path.sourcePath));
                } catch (IOException e) {
                  System.err.printf("Failed to copy '%s' referenced by library '%s' into '%s'\n", path.sourcePath, lib.name, targetPath);
                }
              }
            } else {
              System.err.printf("Deployed file '%s' referenced by library '%s' manifest does not exist\n", path.sourcePath, lib.name);
            }
          }
        }
      }
    }
  }

  private void generateProjectLibraryIncludes(Library lib, String platformName, String configName, GeneratedFile f) {


  }


}
