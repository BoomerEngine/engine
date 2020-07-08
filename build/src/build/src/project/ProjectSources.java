package project;

import project.dependencies.Dependency;
import project.dependencies.DependencyType;
import utils.ProjectException;

import java.nio.file.Path;
import java.util.ArrayList;
import java.util.Collections;
import java.util.List;
import java.util.Optional;

public class ProjectSources extends Project {

  public List<File> files = Collections.synchronizedList(new ArrayList<>());

  public File buildFile;
  public File buildHeader;

  //--

  ProjectSources(Solution sol, Group group, Path absoluteDir, Module module) {
    super(sol, group, absoluteDir, module, ProjectType.SOURCES);
    scanContent();
  }

  private void scanContent() {
    // scan content
    try {
      scanFiles(absolutePath);
    } catch (RuntimeException e) {
      throw new ProjectException(String.format("Scanning of content for project files at '%s; had problems: %s", absolutePath, e.toString())).location(absolutePath);
    }

    // sort the files
    Collections.sort(files, (a,b) -> (a.absolutePath.toString().compareTo(b.absolutePath.toString()))); // make the generator deterministic

    // get the build file
    this.buildFile = files.stream().filter(x -> x.shortName.equals("build.cpp")).findAny().get(); // must exist since we checked for it's existence
    this.buildFile.attributes.addValue("pch", "generate");
    this.attributes = this.buildFile.attributes;

    // get the build header
    this.buildHeader = files.stream().filter(x -> x.shortName.equals("build.h")).findAny().get(); // must exist since we checked for it's existence

    // disable the precompiled header generation on all C files
    this.files.stream()
            .filter(f -> f.shortName.endsWith(".c"))
            .forEach(f -> f.attributes.addValue("pch", "disable"));

    // do we have tests ?
    if (this.files.stream().anyMatch(f -> f.shortName.endsWith("_test.cpp")))
      this.attributes.addValue("hasTests", "1");

    // determine project dependencies
    for (String target : attributes.values("dependency")) {
      Dependency dep = new Dependency(this, DependencyType.PROJECT, target);
      dependencies.add(dep);
    }

    // determine public libraries
    for (String target : attributes.values("publiclib")) {
      Dependency dep = new Dependency(this, DependencyType.PUBLIC_LIBRARY, target);
      dependencies.add(dep);
    }

    // determine private libraries
    for (String target : attributes.values("privatelib")) {
      Dependency dep = new Dependency(this, DependencyType.PRIVATE_LIBRARY, target);
      dependencies.add(dep);
    }
  }

  private void scanFiles(Path absolutePath) {
    utils.Utils.getFilePaths(absolutePath).parallelStream().forEach(childPath -> scanFile(childPath));
    utils.Utils.getSubDirPaths(absolutePath).parallelStream().forEach(childPath -> scanFiles(childPath));
  }

  private void scanFile(Path fileAbsolutePath) {
    Optional<FileType> fileType = FileType.mapExtension(fileAbsolutePath);
    if (fileType.isPresent()) { // we are only interested in files of types we can process
      try {
        Path fileRelativePath = absolutePath.relativize(fileAbsolutePath);
        File file = File.CreatePhysicalFile(fileAbsolutePath, fileRelativePath, fileType.get());

        synchronized (files) {
          files.add(file);
        }
      } catch (ProjectException e) {
        throw new RuntimeException(e);
      }
    }
  }

  public void excludeFromBuild() {
    if (enabled){
      System.out.println(String.format("Excluding project %s from solution generation", mergedName));
      enabled = false;
    }
  }

  //----


}
