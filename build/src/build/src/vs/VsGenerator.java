package vs;

import generators.Generator;

import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.nio.file.StandardCopyOption;
import java.nio.file.attribute.FileTime;
import java.util.*;
import java.util.stream.Collectors;
import java.util.stream.Stream;

public abstract class VsGenerator implements Generator {

  private String toolset;
  private String projectVer;

  public VsGenerator(String toolset, String projectVer) {
    this.toolset = toolset;
    this.projectVer = projectVer;
  }

  //--------------


  //--------------

  /*
   * generate the solution file
   */



}
