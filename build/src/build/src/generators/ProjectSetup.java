package generators;

import com.google.common.collect.Sets;
import library.Library;
import project.File;
import project.FileType;
import project.Project;
import project.ProjectSources;
import project.dependencies.Dependency;
import project.dependencies.DependencyType;
import utils.GeneratedFilesCollection;
import utils.KeyValueTable;

import java.nio.file.Path;
import java.util.ArrayList;
import java.util.List;
import java.util.Set;
import java.util.HashSet;
import java.util.Map;
import java.util.TreeMap;
import java.util.stream.Collectors;

public abstract class ProjectSetup implements Comparable {

  //---

  public String shortName;
  public String mergedName;
  public String solutionPath;
  public KeyValueTable attributes = new KeyValueTable();

  public SolutionSetup solutionSetup; // shared settings

  public Path localOutputPath; // output/projects/<projectname>/
  public Path localGeneratedPath; // output/generated/<projectname>/
  public Path localTempPath; // output/temp/

  public List<ProjectSetup> projectDependencies = new ArrayList<ProjectSetup>();
  public List<Library> publicLibraryDependencies = new ArrayList<Library>();
  public List<Library> privateLibraryDependencies = new ArrayList<Library>();

  private List<ProjectSetup> collectedDependencies = null;
  private List<ProjectSourcesSetup> collectedSourceDependencies = null;

  public Path generatedProjectPath;

  //---

  public ProjectSetup(SolutionSetup solutionSetup, String shortName, String mergedName, String solutionPath) {
    this.solutionSetup = solutionSetup;

    this.mergedName = mergedName;
    this.shortName = shortName;
    this.solutionPath = solutionPath;

    this.localOutputPath = solutionSetup.rootOutputProjectsPath.resolve(mergedName + "/").normalize();
    this.localGeneratedPath = solutionSetup.rootGeneratedPath.resolve(mergedName + "/").normalize();
    this.localTempPath = solutionSetup.rootOutputTempPath.resolve(mergedName + "/").normalize();
  }

  @Override
  public int compareTo(Object o) {
    return mergedName.compareTo(((ProjectSetup)o).mergedName);
  }

  //---

  public abstract void generateFiles(GeneratedFilesCollection files);

  //---

  private static void ExploreProjectDependencies(ProjectSetup project, Map<ProjectSetup, Integer> depthMap, int currentDepth, Set<ProjectSetup> history) {
    // empty projects cannot be a dependency
    if (project == null)
      return;

    // get current depth value from the map
    Integer depth = depthMap.get(project);
    if (depth == null)
      depth = -1;

    // detect recursive cases
    if (history.contains(project)) {
      System.out.printf("Recursive project dependencies on %s\n", project.mergedName);
      return;
    }

    // add from history to prevent recursive visits
    history.add(project);

    // if we need project at some later stage move it
    if (currentDepth > depth) {
      // update the depth value for the project
      depthMap.put(project, currentDepth);

      // visit all project dependencies
      for (ProjectSetup dep : project.projectDependencies)
        ExploreProjectDependencies(dep, depthMap, currentDepth + 1, history);
    }

    // remove from history
    history.remove(project);
  }

  //--


  public List<ProjectSetup> collectSortedDependencies() {
    if (null == collectedDependencies) {
      // prepare map
      Map<ProjectSetup, Integer> depthMap = new TreeMap<>();

      // explore dependencies starting from the specified project
      Set<ProjectSetup> history = new HashSet<>();
      ExploreProjectDependencies(this, depthMap, 0, history);

      // assemble in dependency order
      collectedDependencies = depthMap.entrySet().stream()
              .filter(e -> e.getValue() > 0) // skip the ones that were not visited
              .sorted((a, b) -> Integer.compare(b.getValue(), a.getValue())) // the deeper in the dependency graph we are the sooner we should be reported
              .map(e -> e.getKey())
              .filter(e -> !e.attributes.hasKey("app")) // applications cannot be dependencies
              .collect(Collectors.toList());

      //for (ProjectSetup p : collectedDependencies) {
        //System.out.printf("Dependency '%s' on '%s'\n", mergedName, p.mergedName);
      //}
    }

    return collectedDependencies;
  }

  //--

  public List<ProjectSourcesSetup> collectSortedSourcesDependencies() {
    if (null == collectedSourceDependencies) {
      collectedSourceDependencies = new ArrayList<ProjectSourcesSetup>();

      for (ProjectSetup dep : collectSortedDependencies()) {
        if (dep instanceof ProjectSourcesSetup) {
          ProjectSourcesSetup sourceDep = (ProjectSourcesSetup) dep;
          if (sourceDep != null)
            collectedSourceDependencies.add(sourceDep);
        }
      }
    }

    return collectedSourceDependencies;
  }

  public List<Library> collectLibraries(String type) {
    Set<Library> ret = new HashSet<>();

//    System.out.printf("Collecting %s libraries for %s\n", type, mergedName);

    if (type.equals("internal")) {
      for (ProjectSetup dep : collectSortedDependencies())
        for (Library l : dep.publicLibraryDependencies)
          ret.add(l);

      for (Library l : publicLibraryDependencies)
        ret.add(l);

      for (Library l : privateLibraryDependencies)
        ret.add(l);
    }
    else if (type.equals("all")) {
      for (ProjectSetup dep : collectSortedDependencies()) {
        for (Library l : dep.publicLibraryDependencies)
          ret.add(l);
        for (Library l : dep.privateLibraryDependencies)
          ret.add(l);
      }

      for (Library l : publicLibraryDependencies)
        ret.add(l);

      for (Library l : privateLibraryDependencies)
        ret.add(l);
    }

    //for (Library l : ret)
      //System.out.printf("Collected library %s\n", l.name);

    List<Library> ret2 = new ArrayList<Library>();
    ret2.addAll(ret);
    ret2.sort((o1,o2) -> o1.absolutePath.compareTo(o2.absolutePath));
    return ret2;
  }

}
