package project;

import com.google.common.primitives.Booleans;
import generators.PlatformType;
import generators.SolutionType;
import library.LibraryManager;
import project.dependencies.*;

import java.nio.file.Path;
import java.nio.file.Paths;
import java.util.*;
import java.util.stream.Collectors;

public class Solution {

  //---

  public SolutionType solutionType = SolutionType.FULL;
  public PlatformType platformType = PlatformType.WINDOWS;
  public List<Path> sourceCodeRootPaths;
  public LibraryManager libs;
  public List<Group> rootGroups = Collections.synchronizedList(new ArrayList<>());
  public List<Project> projects = new ArrayList<>();
  //public List<ProjectSources> sourcesProjects = new ArrayList<>();

  public Path mainProjectPath;
  public Path buildOutputPath;
  public Path buildPublishPath;

  public boolean engineSolution;

  public Path generatedSolutionFile = null;

  //---

  public Solution(SolutionType solutionType, PlatformType platform, LibraryManager libs, Path mainProjectPath, Path outputPath, Path publishPath, boolean engineSolution) {
    this.solutionType = solutionType;
    this.platformType = platform;
    this.libs = libs;
    this.buildOutputPath = outputPath.normalize();
    this.buildPublishPath = publishPath.normalize();
    this.mainProjectPath = mainProjectPath;
    this.engineSolution = engineSolution;

    System.out.printf("Project build files will be written to: '%s'\n", this.buildOutputPath.toString());
    System.out.printf("Project binary files will be written to: '%s'\n", this.buildPublishPath.toString());
  }

  //---

  public Project projectByName(String projectName) {
    for (Project p : projects)
      if (p.mergedName.equals(projectName))
        return p;
    return null;
  }

  public void scanModuleContent(Module owningModule) {
    scanProjectGroups(owningModule.sourceDirectory, null, owningModule);
  }

  public void finalizeScannedContent() {
    // make the groups deterministic
    rootGroups.sort((a,b) -> a.shortName.compareTo(b.shortName));

    // collect final projects
    for (Group g : rootGroups) {
      g.makeDeterministic();
      g.collectAllProjects(projects);
    }

    // count files in all projects
    int totalFiles = 0;
    for (Project p : projects)
      if (p instanceof  ProjectSources)
        totalFiles += ((ProjectSources)p).files.size();

    // prints stats
    System.out.println(String.format("Collected %d projects in total", projects.size()));
    System.out.println(String.format("Collected %d files in total", totalFiles));
  }

  public void collectProjectDependencies(Project project, boolean recursive, DependencyType dependencyType, Set<Dependency> ret, Set<Project> history) {
    // collect public dependencies from the project itself
    project.dependencies.stream()
            .filter(dep -> dep.type == dependencyType)
            .collect(Collectors.toCollection(() -> ret));

    // prevent infinite recursion
    if (history.contains(project))
      return;

    // add to history
    history.add(project);

    // visit other projects
    if (recursive) {
      for (Dependency dep : project.dependencies) {
        if (dep.type == DependencyType.PROJECT) {
          for (Project p : dep.resolvedProjects) {
            collectProjectDependencies(p, recursive, dependencyType, ret, history);
          }
        }
      }
    }

    // remove from history
    // history.remove(project);
  }

  public Set<Dependency> collectProjectDependencies(Project project, boolean recursive, DependencyType dependencyType) {
    Set<Dependency> ret = new HashSet<Dependency>();
    Set<Project> history = new HashSet<>();
    collectProjectDependencies(project, recursive, dependencyType, ret, history);
    return ret;
  }

  //---

  public void resolveDependencies(LibraryManager libraries) {
    // resolve all content
    SolutionDependencyResolver resolver = new SolutionDependencyResolver(this, libraries);
    for (Project p : projects) {
      System.out.println(String.format("Resolving %d dependencies for %s", p.dependencies.size(), p.mergedName));
      for (Dependency d : p.dependencies) {
        d.resolveDependency(resolver);
      }
    }

    // TODO: exclude projects based on platform

    // some engine-only projects are not generated when we have a project specified (ie. rendering tests, general tests, some internal wireing etc)
    if (!solutionType.allowEngineProjects || !engineSolution) {
      System.out.println("Disabling engine only projects...");
      for (Project p : projects) {
        if (p instanceof ProjectSources) {
          if (p.attributes.hasKey("engineonly")) {
            p.excludeFromBuild();
          }
        }
      }
    }

    // in final mode all of the dev-only (builders, importers, editors, etc) projects are excluded
    if (!solutionType.allowDevProjects) {
      System.out.println("Disabling dev only projects...");
      for (Project p : projects) {
        if (p instanceof ProjectSources) {
          if (p.attributes.hasKey("devonly")) {
            p.excludeFromBuild();
          }
        }
      }
    }

    // do refinement passes
    int removedAdditionalProjects = 0;
    for (int pass=0; pass<projects.size(); ++pass) {
      boolean somethingChanged = false;
      for (Project p : projects) {
        if (p.enabled) {
          for (Dependency d : p.dependencies) {
            d.validateDependency();
          }
          if (!p.enabled) {
            removedAdditionalProjects += 1;
            somethingChanged = true;
          }
        }
      }

      if (!somethingChanged) {
        System.out.println(String.format("Finished dependency propagation after %d passes, removed %d additional projects", pass+1, removedAdditionalProjects));
        break;
      }
    }

    // list projects that are removed
    {
      int numExcludedProjects = 0;
      for (Project p : projects) {
        if (!p.enabled)
          numExcludedProjects += 1;
      }

      if (numExcludedProjects > 0){
        System.out.println(String.format("Solution will NOT contain following %d projects because they were either disabled or have invalid dependencies:", numExcludedProjects));
        for (Project p : projects) {
          if (!p.enabled) {
            System.out.println(String.format("Project '%s' loaded from %s", p.mergedName, p.absolutePath.toString()));
          }
        }
      }
    }
  }

  //---

  public Group createRootGroup(String name) {
    synchronized (rootGroups) {
      for (Group g : rootGroups)
        if (g.shortName.equals(name))
          return g;

      Group g = new Group(this, null, name);
      rootGroups.add(g);
      return g;
    }
  }

  public Group createProjectGroup(Group parentGroup, String name) {
    if (parentGroup == null) {
      return createRootGroup(name);
    } else {
      return parentGroup.createChildGroup(name);
    }
  }

  private static boolean IsProjectPath(Path rootDirectory) {
    java.io.File file = new java.io.File(rootDirectory.resolve("src/build.cpp").toString());
    return file.exists();
  }

  private void scanProjectGroups(Path path, Group parentGroup, Module module) {
    for (Path subDirPath : utils.Utils.getSubDirPaths(path)) {
      // if we encounter a project directory our torment has ended and we can create a project
      if (IsProjectPath(subDirPath)) {
        parentGroup.createSourcesProject(subDirPath, module);
      } else {
        createGroupAtPath(subDirPath, parentGroup, module);
      }
    }
  }

  private void createGroupAtPath(Path groupPath, Group parentGroup, Module module) {
    String groupName = groupPath.getFileName().toString();
    Group group = createProjectGroup(parentGroup, groupName);
    scanProjectGroups(groupPath, group, module);
  }

  //---
}
