package vs;

import generators.PlatformType;
import generators.ProjectSetup;
import generators.SolutionSetup;
import project.Project;
import project.ProjectSources;
import project.Solution;
import project.dependencies.Dependency;
import project.dependencies.DependencyType;
import utils.GeneratedFile;
import utils.GeneratedFilesCollection;
import utils.KeyValueTable;
import utils.Utils;

import java.io.IOException;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Set;
import java.util.Map;
import java.util.stream.Collectors;

public class VsSolutionSetup extends SolutionSetup {

  public Path solutionPath;

  public String toolset;
  public String projectVersion;

  //---

  public VsSolutionSetup(Solution baseSolution, KeyValueTable params, String toolset, String projectVersion) throws IOException {
    super(baseSolution, params);

    String rootModuleSolution = baseSolution.solutionType.toString().toLowerCase()  + ".sln";

    this.toolset = toolset;
    this.projectVersion = projectVersion;
    this.solutionPath = rootOutputPath.resolve(rootModuleSolution);
    baseSolution.generatedSolutionFile = this.solutionPath;

    this.buildToolsPath = utils.Utils.getCurrentJar().getParentFile().toPath().normalize(); // gets the \build\tools\bin\
    this.buildScriptsPath = buildToolsPath.resolve("build/vs/").normalize();

    this.allPlatforms.add("x64");

    this.allConfigs.add("Debug");
    this.allConfigs.add("Checked");
    this.allConfigs.add("Release");

    System.out.printf("Build tools path: '%s'\n", buildToolsPath.toString());
    System.out.printf("Build scripts path: '%s'\n", buildScriptsPath.toString());

    // create new project stubs
    Map<Project, ProjectSetup> originalToProject = new HashMap<>();
    Map<ProjectSetup, Project> projectToOriginal = new HashMap<>();
    for (Project p : baseSolution.projects) {
      ProjectSetup vsp = null;

      if (p.enabled) {
        ProjectSources ps = (ProjectSources)p;
        if (ps != null) {
          vsp = new VsProjectSourcesSetup(ps, this);
        }
      }

      if (vsp != null) {
        originalToProject.put(p, vsp);
        projectToOriginal.put(vsp, p);
        projects.add(vsp);
      }
    }

    // copy dependencies from input solution
    for (Project p : baseSolution.projects) {
      ProjectSetup vsp = originalToProject.get(p);
      if (vsp != null) {
        for (Dependency dep : p.dependencies) {
          for (Project dependencyProject : dep.resolvedProjects) {
            ProjectSetup depVsp = originalToProject.get(dependencyProject);
            if (depVsp != null) {
              vsp.projectDependencies.add(depVsp);
            }
          }
        }
      }
    }

    // create the rtti generator project
    {
      VsProjectRTTIGenSetup rttiGen = new VsProjectRTTIGenSetup(this);
      for (ProjectSetup p : projects) {
        p.collectSortedSourcesDependencies();
        p.projectDependencies.add(rttiGen);
      }
      System.out.printf("Created _rtti_gen project with %d dependencies\n", projects.size());
      projects.add(rttiGen);
    }

  }

  //---

  @Override
  public void generateFiles(GeneratedFilesCollection files) {
    // only windows platform is supported
    if (platformType != PlatformType.WINDOWS)
      throw new RuntimeException("Only windows platform is supported for Visual Studio generators (ATM)");

    // Generate the project content
    // NOTE: we only generate content for enabled projects
    projects.parallelStream().forEach(p -> p.generateFiles(files));

    // Generate the RTTI glue code
    generateRTIIManifest(files);

    // generate the glued header
    generateGluedBigHeader(files);

    // Generate the solution
    generateSolution(files);
  }

  //--

  private void generateSolution(GeneratedFilesCollection files) {
    // create a writer
    GeneratedFile f = files.createFile(solutionPath);
    // header
    f.writeln("Microsoft Visual Studio Solution File, Format Version 12.00");
    if (toolset.equals("v140")) {
      f.writeln("# Visual Studio 14");
      f.writeln("# Generated file, please do not modify");
      f.writeln("VisualStudioVersion = 14.0.25420.1");
      f.writeln("MinimumVisualStudioVersion = 10.0.40219.1");
    } else {
      f.writeln("# Visual Studio 15");
      f.writeln("# Generated file, please do not modify");
      f.writeln("VisualStudioVersion = 15.0.26403.7");
      f.writeln("MinimumVisualStudioVersion = 10.0.40219.1");
    }

    // generate groups
    generateSolutionGroups(f);

    // begin global section
    f.writeln("Global");

    // generate config table (Release, Debug, etc)
    generateSolutionConfigTable(f);

    // generate solution properties
    generateSolutionProperties(f);

    // generate project parenting structure
    ggenerateSolutionNestedProjects(f);

    // finish global section
    f.writeln("EndGlobal");
  }

  private static List<String> SetToSortedList(Set<String> groupNames) {
    List<String> ret = new ArrayList<String>();
    ret.addAll(groupNames);
    ret.sort((o1,o2) -> o1.compareTo(o2));
    return ret;
  }

  private static List<String> CollectRootGroupNames(List<String> groupNames) {
    Set<String> roots = new HashSet<String>();
    for (String groupName : groupNames) {
      int index = groupName.indexOf('.');
      if (index != -1)
        groupName = groupName.substring(0, index);
      roots.add(groupName);
    }

    return SetToSortedList(roots);
  }

  private static List<String> CollectChildGroupNames(List<String> groupNames, String rootName) {
    List<String> ret = new ArrayList<String>();
    for (String groupName : groupNames) {
      if (groupName.startsWith(rootName + ".")) {
        String childGroupName = groupName.substring(rootName.length() + 1);
        if (!childGroupName.isEmpty())
          ret.add(childGroupName);
      }
    }
    return ret;
  }

  private List<String> collectAllSolutionGroupNames() {
    Set<String> groupStrings = new HashSet<>();
    for (ProjectSetup p : projects) {
      if (!p.solutionPath.isEmpty())
        groupStrings.add(p.solutionPath);
    }
    return SetToSortedList(groupStrings);
  }

  static class GroupEntry {
    String parentGuid;
    String childGuid;
  }

  private List<GroupEntry> solutionGroupParentEntries = new ArrayList<>();

  private void generateSolutionGroupProject(GeneratedFile f, String baseName, String parentGuid, List<String> groupNames) {
    for (String rootName : CollectRootGroupNames(groupNames)) {

      String guid = Utils.guidFromText("group_" + baseName + rootName);

      if (parentGuid != null) {
        GroupEntry entry = new GroupEntry();
        entry.childGuid = guid;
        entry.parentGuid = parentGuid;
        solutionGroupParentEntries.add(entry);
      }

      f.writelnf("Project(\"{2150E333-8FDC-42A3-9474-1A3956D46DE8}\") = \"%s\", \"%s\", \"%s\"", rootName, rootName, guid);
      f.writeln("EndProject");

      generateSolutionGroupProject(f, baseName + rootName + ".", guid, CollectChildGroupNames(groupNames, rootName));
    }
  }

  private void generateSolutionGroups(GeneratedFile f) {
    generateSolutionGroupProject(f, "", null, collectAllSolutionGroupNames());

    for (ProjectSetup p : projects) {
      String guid = Utils.guidFromText("project_" + p.mergedName);

      if (!p.solutionPath.isEmpty()) {
        GroupEntry entry = new GroupEntry();
        entry.childGuid = guid;
        entry.parentGuid =  Utils.guidFromText("group_" + p.solutionPath);
        solutionGroupParentEntries.add(entry);
      }

      f.writelnf("Project(\"{8BC9CEB8-8B4A-11D0-8D11-00A0C91BC942}\") = \"%s\", \"%s\", \"%s\"", p.mergedName, p.generatedProjectPath, guid);
      f.writeln("EndProject");
    }
  }

  private void generateSolutionConfigTable(GeneratedFile f) {
    // solution configs
    f.writeln("	GlobalSection(SolutionConfigurationPlatforms) = preSolution");

    for (String c : allConfigs)
      for (String p : allPlatforms)
        f.writelnf("		%s|%s = %s|%s", c, p, c, p);

    f.writeln("	EndGlobalSection");

    // project configs
    f.writeln("	GlobalSection(ProjectConfigurationPlatforms) = postSolution");

     for (ProjectSetup proj : projects) {
       for (String c : allConfigs) {
         for (String p : allPlatforms) {
           String projectGuid = Utils.guidFromText("project_" + proj.mergedName);
           f.writelnf("		%s.%s|%s.ActiveCfg = %s|%s", projectGuid, c, p, c, p);
           f.writelnf("		%s.%s|%s.Build.0 = %s|%s", projectGuid, c, p, c, p);
         }
       }
     }

    f.writeln("	EndGlobalSection");
  }

  private void generateSolutionProperties(GeneratedFile f) {
    f.writeln("	GlobalSection(SolutionProperties) = preSolution");
    f.writeln("		HideSolutionNode = FALSE");
    f.writeln("	EndGlobalSection");
  }

  private void ggenerateSolutionNestedProjects(GeneratedFile f) {
    f.writeln("	GlobalSection(NestedProjects) = preSolution");

    for (GroupEntry entry : solutionGroupParentEntries) {
      // {808CE59B-D2F0-45B3-90A4-C63953B525F5} = {943E2949-809F-4411-A11F-51D51E9E579B}
      f.writelnf("%s = %s", entry.childGuid, entry.parentGuid);
    }

    f.writeln("	EndGlobalSection");
  }
}
