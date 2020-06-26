package utils;

import org.w3c.dom.Document;
import org.w3c.dom.Element;
import org.w3c.dom.NamedNodeMap;
import org.w3c.dom.Node;
import org.xml.sax.SAXException;

import javax.xml.parsers.DocumentBuilder;
import javax.xml.parsers.DocumentBuilderFactory;
import javax.xml.parsers.ParserConfigurationException;
import java.io.File;
import java.io.IOException;
import java.lang.reflect.Modifier;
import java.net.URL;
import java.net.URLClassLoader;
import java.nio.ByteBuffer;
import java.nio.charset.Charset;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.nio.file.StandardOpenOption;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.Optional;
import java.util.jar.JarFile;
import java.util.stream.Collectors;
import java.util.zip.CRC32;

/**
 * Created by RexDex on 2/26/2017.
 */
public class Utils {

  //---

  public static <T> void mergeList(List<T> list, List<T> contentToMerge) {
    contentToMerge.stream()
            .filter(p -> !list.contains(p))
            .collect(Collectors.toCollection(() -> list));
  }

  //---

  private static int getPartialGUID(String s) {
    CRC32 c = new CRC32();
    c.update(s.getBytes());
    return (int)c.getValue();
  }

  public static String guidFromText(String txt) {
    byte[] bytes = ByteBuffer.allocate(16).
            putInt(getPartialGUID("part1_" + txt)).
            putInt(getPartialGUID("part2_" + txt)).
            putInt(getPartialGUID("part3_" + txt)).
            putInt(getPartialGUID("part4_" + txt)).array();

    return String.format("{%02X%02X%02X%02X-%02X%02X-%02X%02X-%02X%02X-%02X%02X%02X%02X%02X%02X}",
            bytes[0], bytes[1], bytes[2], bytes[3], bytes[4], bytes[5], bytes[6], bytes[7],
            bytes[8], bytes[9], bytes[10], bytes[11], bytes[12], bytes[13], bytes[14], bytes[15]);
  }

  public static Document loadXML(Path filePath) {
    try {
      File xmlFile = new File(filePath.toString());
      DocumentBuilderFactory dbFactory = DocumentBuilderFactory.newInstance();
      DocumentBuilder builder = dbFactory.newDocumentBuilder();
      return builder.parse(xmlFile);
    } catch (java.io.FileNotFoundException e) {
      return null;
    } catch (IOException e) {
      throw new ProjectException("Failed to open file", e).location(filePath);
    } catch (SAXException e) {
      throw new ProjectException("Failed to parse XML document", e).location(filePath);
    } catch (ParserConfigurationException e) {
      throw new ProjectException("Failed to parse XML document", e).location(filePath);
    }
  }

  public static List<Element> getXmlNodes(Element node, String name) {
    List<Element> ret = new ArrayList<>();

    if (node != null) {
      Node p = node.getFirstChild();

      for (; p != null; p = p.getNextSibling()) {
        if (p.getNodeType() != Node.ELEMENT_NODE)
          continue;

        if (!p.getNodeName().equals(name))
          continue;

        ret.add((Element) p);
      }
    }

    return ret;
  }

  public static Element getXmlNode(Element node, String name) {
    if (node != null) {
      Node p = node.getFirstChild();

      for (; p != null; p = p.getNextSibling()) {
        if (p.getNodeType() != Node.ELEMENT_NODE)
          continue;

        if (!p.getNodeName().equals(name))
          continue;

        return (Element)p;
      }
    }

    return null;
  }

  public static KeyValueTable getXmlNodeAttributes(Element node) {
    KeyValueTable ret = new KeyValueTable();

    if (node != null) {
      NamedNodeMap attrs = node.getAttributes();
      for (int i=0; i<attrs.getLength(); ++i) {
        Node attr = attrs.item(i);
        ret.addValue(attr.getNodeName(), attr.getNodeValue());
      }
    }

    return ret;
  }

  public static List<Path> getSubDirPaths(Path absolutePath) {
    java.io.File curDir = new java.io.File(absolutePath.toString());
    assert curDir.isDirectory();

    if (curDir.list() == null)
      return new ArrayList<Path>();

    return Arrays.stream(curDir.listFiles())
            .filter(x -> x.isDirectory())
            .map(x -> Paths.get(x.getPath()))
            .sorted((o1, o2)->o1.compareTo(o2))
            .collect(Collectors.toList());
  }

  public static List<Path> getFilePaths(Path absolutePath) {
    java.io.File curDir = new java.io.File(absolutePath.toString());
    assert curDir.isDirectory();

    if (curDir.list() == null)
      return new ArrayList<Path>();

    return Arrays.stream(curDir.listFiles())
            .filter(x -> x.isFile())
            .map(x -> Paths.get(x.getPath()))
            .sorted((o1, o2)->o1.compareTo(o2))
            .collect(Collectors.toList());
  }

  //---

  /*
   * Get path to the current JAR we are it
   */
  public static File getCurrentJar() throws IOException {
    File jarFile = new File(Utils.class.getProtectionDomain().getCodeSource().getLocation().getPath());
    if (!jarFile.getPath().endsWith(".jar"))
      throw new IOException("Program not run from JAR file");

    return jarFile;
  }

  //--

  /*
   * load content of file to string
   */
  public static String loadFileToString(Path path, Charset encoding) throws IOException {
    byte[] encoded = Files.readAllBytes(path); // TODO: optimize! we don't have to load the full content
    return new String(encoded, encoding);
  }

  /*
   * save string to file
   */
  public static void saveStringToFile(Path path, String txt, Charset encoding) throws IOException {
    File f = new File(path.toString());
    f.getParentFile().mkdirs();
    f.createNewFile();

    byte[] encoded = txt.getBytes(encoding);
    Files.write(path, encoded, StandardOpenOption.TRUNCATE_EXISTING);
  }

  /*
   * Update file timestamp
   */


  public static void touchFile(Path path) throws IOException {
    File file = new File(path.toString());
    long timestamp = System.currentTimeMillis();
    file.setLastModified(timestamp);
  }
}
