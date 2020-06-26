package vs;

import generators.ProjectSetup;
import generators.SolutionSetup;
import project.File;
import project.FileType;
import project.ProjectSources;
import utils.GeneratedFile;
import utils.GeneratedFilesCollection;
import utils.ProjectException;
import utils.Utils;

import java.io.BufferedReader;
import java.io.InputStreamReader;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.util.*;
import java.util.stream.Collectors;

public class VsProjectRTTIGenSetup extends ProjectSetup {

  //---

  public String toolset;
  public String projectVersion;

  public VsProjectRTTIGenSetup(VsSolutionSetup solutionSetup) {
    super(solutionSetup, "_rtti_gen", "_rtti_gen", "BoomerEngine");

    this.toolset = solutionSetup.toolset;
    this.projectVersion = solutionSetup.projectVersion;
    this.generatedProjectPath = this.localOutputPath.resolve(mergedName + ".vcxproj");
  }

  //---

  @Override
  public void generateFiles(GeneratedFilesCollection files)  {
    GeneratedFile f = files.createFile(generatedProjectPath);
    System.out.printf("Generating _rtti_gen project");

    f.writeln("<?xml version=\"1.0\" encoding=\"utf-8\"?>");
    f.writeln("<!-- Auto generated file, please do not edit -->");

    f.writelnf("<Project DefaultTargets=\"Build\" ToolsVersion=\"%s\" xmlns=\"http://schemas.microsoft.com/developer/msbuild/2003\">", projectVersion);

    f.writelnf("<Import Project=\"%s\\SharedConfigurationSetup.props\"/>", solutionSetup.buildScriptsPath);

    f.writeln("<PropertyGroup>");
    f.writelnf("  <PlatformToolset>%s</PlatformToolset>", toolset);
    f.writelnf("  <VCProjectVersion>%s</VCProjectVersion>", projectVersion);
    f.writeln("  <WindowsTargetPlatformVersion>10.0</WindowsTargetPlatformVersion>");
    f.writeln(" 	<ModuleType>Empty</ModuleType>");
    f.writeln(" 	<SolutionType>Dev</SolutionType>");
    f.writelnf(" 	<ProjectGuid>%s</ProjectGuid>", Utils.guidFromText("project_" + mergedName));
    f.writelnf(" 	<DisableFastUpToDateCheck>true</DisableFastUpToDateCheck>");
    f.writeln("</PropertyGroup>");
    f.writeln("  <PropertyGroup>");
    f.writeln("    <PreBuildEventUseInBuild>true</PreBuildEventUseInBuild>");
    f.writeln("  </PropertyGroup>");
    f.writeln("  <ItemDefinitionGroup>");
    f.writeln("    <PreBuildEvent>");

    try {
      Path buildPath = utils.Utils.getCurrentJar().toPath().normalize();
      f.writelnf("      <Command>java -jar \"%s\" -command=rtti -project=\"%s\" -solution=\"%s\"</Command>", buildPath, solutionSetup.mainProjectPath, solutionSetup.solutionType.name);
    } catch (Exception e) {

    }

    f.writeln("    </PreBuildEvent>");
    f.writeln("  </ItemDefinitionGroup>");
    f.writelnf("<Import Project=\"%s\\SharedItemGroups.props\"/>", solutionSetup.buildScriptsPath);
    f.writeln("  <ItemGroup>");
    f.writeln("  </ItemGroup>");
    f.writeln("  <ItemGroup>");
    f.writeln("  </ItemGroup>");
    f.writelnf(" <Import Project=\"%s\\Shared.targets\"/>", solutionSetup.buildScriptsPath);
    f.writeln(" <Import Project=\"$(VCTargetsPath)\\Microsoft.Cpp.targets\"/>");
    f.writeln("</Project>");
  }

}
