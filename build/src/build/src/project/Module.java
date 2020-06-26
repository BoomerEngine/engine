package project;

import library.Library;
import utils.KeyValueTable;
import utils.ProjectException;

import java.nio.file.Path;
import java.nio.file.Paths;
import java.util.ArrayList;
import java.util.List;
import java.nio.file.*;

public class Module {

  public final ModuleManifest manifest;

  public Path rootDirectory;
  public Path sourceDirectory;
  public Path dataDirectory;
  public int dependencyLevel;

  public Module(Path rootDirectory, ModuleManifest manifest, int level) {
    this.manifest = manifest;
    this.rootDirectory = rootDirectory.normalize();
    this.dependencyLevel = level;

    if (Files.exists(this.rootDirectory.resolve("src"))) {
      this.sourceDirectory = this.rootDirectory.resolve("src");
      System.out.printf("Found source directory for module '%s': '%s'\n", manifest.name, this.sourceDirectory);
    }

    if (Files.exists(this.rootDirectory.resolve("data"))) {
      this.dataDirectory = this.rootDirectory.resolve("data");
      System.out.printf("Found data directory for module '%s': '%s'\n", manifest.name, this.dataDirectory);
    }
  }

}
