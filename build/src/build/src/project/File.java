package project;

import generators.PlatformType;
import utils.KeyValueTable;
import utils.ProjectException;

import java.io.IOException;
import java.nio.charset.Charset;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.attribute.FileTime;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Set;
import java.util.HashSet;
import java.util.List;
import java.util.Optional;

public class File {

  public FileType type;

  public String shortName; // dupa.cpp
  public String coreName; // dup
  public Path absolutePath; // Z:\git\boomer\engine\src\base\app\src\dupa.cpp
  public Path relativePath;// src\dupa.cpp

  public Set<PlatformType> platformsFilter = null;
  public FileTime modificationTime;

  public KeyValueTable attributes;

  //--

  // Create a file entry based on a physical file on disk
  public static File CreatePhysicalFile(Path absolutePath, Path relativePath, FileType type) throws ProjectException {
    File ret = new File();

    ret.absolutePath = absolutePath;
    ret.relativePath = relativePath;
    ret.shortName = relativePath.getFileName().toString();
    ret.coreName = ret.shortName.substring(0, ret.shortName.indexOf('.'));
    ret.type = type;

    try {
      // load and process the file preamble
      String filePreamble = readFilePreamble(absolutePath, Charset.defaultCharset());
      ret.attributes = KeyValueTable.parseFromParamBlock(filePreamble);
    } catch (IOException e) {
      System.err.println("Failed to load content of file " + absolutePath + "'");
      throw new ProjectException("Unable to load file").location(absolutePath);
    } catch (ProjectException p) {
      throw p.location(absolutePath);
    }

    if (ret.attributes.hasKey("platform")) {
      ret.platformsFilter = new HashSet<PlatformType>();

      List<String> platformNames = ret.attributes.values("platform");
      for (String platformName : platformNames) {
        List<PlatformType> filterPlatforms = PlatformType.mapFilter(platformName);
        if (filterPlatforms.isEmpty()) {
          System.err.println("File '" + absolutePath + "' uses unknown platform filter '" + platformName + "'");
          throw new ProjectException("Invalid filter platform option").location(absolutePath);
        }
        for (PlatformType filter : filterPlatforms)
          ret.platformsFilter.add(filter);
      }
    }

    return ret;
  }

  // Create a virtual file entry
  public static File CreateVirtualFile(Path absolutePath, Path relativePath, FileType type) throws ProjectException {
    File ret = new File();

    ret.absolutePath = absolutePath;
    ret.relativePath = relativePath;
    ret.shortName = relativePath.getFileName().toString();
    ret.attributes = new KeyValueTable();
    ret.type = type;

    return ret;
  }

  //--

  private static int PREAMBLE_SIZE = 4096;

  //--

  private static String readFilePreamble(Path path, Charset encoding) throws IOException {
    byte[] encoded = Files.readAllBytes(path); // TODO: optimize! we don't have to load the full content
    String ret = new String(encoded, encoding);
    return ret.substring(0, Math.min(PREAMBLE_SIZE, ret.length()));
  }
}
