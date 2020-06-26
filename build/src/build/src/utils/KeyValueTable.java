package utils;

import com.google.common.collect.Lists;
import org.w3c.dom.Document;
import org.w3c.dom.Node;

import java.nio.file.Path;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.Optional;
import java.util.stream.Collectors;

/**
 * Created by RexDex on 2/26/2017.
 */
public class KeyValueTable {
  public KeyValueTable() {
    parameters = new ArrayList<>();
  }

  public static class Attribute
  {
    public String name;
    public String value;

    public Attribute(String nameToSet, String valueToSet)
    {
      name = nameToSet;
      value = valueToSet;
    }
  }


  /*
    Parse the attribute table from a simple embedded attribute table
    This is used inside a text files and looks like that:
    // [# name1: value1, value2, value3 #]
    // [# name2: value #]
   */
  public static KeyValueTable parseFromParamBlock(String txt) {

    KeyValueTable ret = new KeyValueTable();

    Tokenizer tok = new Tokenizer(txt);
    while (tok.findKeyword("[#"))
    {
      // get the name of the parameter
      String name = tok.parseToken();
      if (name.isEmpty()) {
        throw new ProjectException("Expected parameter name after the [#").location(tok.getLineNumber());
      }

      // scan values, may not be defined
      ArrayList<String> values = new ArrayList<>();
      if (tok.parseKeyword(":")) {
        // parse all the values
        while (!tok.parseKeyword("#]")) {
          // we need a separator between values
          if (!values.isEmpty()) {
            if (!tok.parseKeyword(",")) {
              throw new ProjectException("Expected value separator").location(tok.getLineNumber());
            }
          }

          // get value, empty values are NOT supported here
          String value = tok.parseDelimitedToken("#],").trim();
          if (value.isEmpty()) {
            throw new ProjectException("Empty parameter values are not supported").location(tok.getLineNumber());
          }

          values.add(value);
        }
      }

      // emit the attribute
      ret.addValues(name, values);
    }

    return ret;
  }

  /*
    Parse the attribute table from simple XML, the format:
    <properties>
      <key>Name</key>
      <value>Some value</value>
      <key>Flag</key>
      <key>Table</key>
      <value>ArrayValue1</value>
      <value>ArrayValue2</value>
      <value>ArrayValue3</value>
    </properties>
   */
  public static KeyValueTable parseFromXml(Path xmlFilePath) {
    KeyValueTable ret = null;
    Document doc = utils.Utils.loadXML(xmlFilePath);
    if (doc != null) {
      ret = new KeyValueTable();

      try {
        Node p = doc.getDocumentElement().getFirstChild();

        String lastKey = "";
        for (; p != null; p = p.getNextSibling()) {
          if (p.getNodeType() != Node.ELEMENT_NODE)
            continue;

          if (p.getNodeName().equals("key")) {
            lastKey = p.getTextContent();
          } else if (p.getNodeName().equals("value")) {
            if (lastKey.isEmpty()) {
              throw new utils.ProjectException("Value node without a matching key").location(xmlFilePath);
            }

            ret.addValue(lastKey, p.getTextContent());
          } else {
            throw new utils.ProjectException("Unexpected XML node " + p.getNodeName()).location(xmlFilePath);
          }
        }
      } catch (Exception e) {
        throw new utils.ProjectException("XML parsing error", e).location(xmlFilePath);
      }
    }
    return ret;
  }

  /*
   * Parse the attributes from a command line String[] table
   * This assumes the following format:
   * -key=value -key2=value2
   * -key3=value1,value2,value3
   * -key4="crap crap crap"
   */
  public static KeyValueTable parseFromCommandline(String cmdline) {
    KeyValueTable ret = new KeyValueTable();

    // each argument structure:
    // -<ident>=<value>,<value>

    Tokenizer tok = new Tokenizer(cmdline);

    try {
      while (tok.hasContent()) {
        // arguments must start with the "-"
        if (!tok.parseKeyword("-")) {
          throw new utils.ProjectException("Commandline arguments must begin with -");
        }

        // get the required param name
        String name = tok.parseToken();
        if (name.isEmpty()) {
          throw new utils.ProjectException("Unnamed commandline arguments are not supported");
        }

        // if we have the "=" following the name we can expect some zero+ parameters
        ArrayList<String> values = new ArrayList<String>();
        if (tok.parseKeyword("=")) {
          do {
            // we expect a value separator after each element if we want to continue
            String value = tok.parseDelimitedToken(",-").trim(); // we parse until we find a limiting char (or we run out of chars)
            values.add(value); // note: this allows empty values
          } while (tok.parseKeyword(","));
        }

        // we have a name and list of values, add them to param table
        ret.addValues(name, values);
      }
    } catch (ProjectException e) {
      throw new utils.ProjectException("Commandline parsing error", e);
    }

    return ret;
  }

  /*
   * Parse the attributes from a command line String[] table
   * Reassembles the commandline into one big string so it's parsed more safely
  */
  public static KeyValueTable parseFromCommandline(String[] args) {
    return parseFromCommandline( Arrays.stream(args).reduce("", (a,b) -> a + " " + b));
  }

  //---

  public boolean hasKey(String name) {
    for (Attribute param : parameters)
      if (param.name.equals(name))
        return true;
    return false;
  }

  public String value(String name) {
    for (Attribute param : parameters)
      if (param.name.equals(name))
        return param.value;
    return "";
  }

  public String valueOrDefault(String name, String defaultValue) {
    for (Attribute param : parameters)
      if (param.name.equals(name))
        return param.value;
    return defaultValue;
  }

  public List<String> values(String name) {
    return parameters.stream()
            .filter(p -> p.name.equals(name))
            .map(p -> p.value)
            .collect(Collectors.toList());
  }

  public void setValue(String name, String value) {
    removeValue(name);
    addValue(name, value);
  }

  public void addValue(String name, String value) {
    parameters.add(new Attribute(name, value));
  }

  public void addValues(String name, ArrayList<String> values) {
    if (values.isEmpty()) {
      addValue(name, "");
    } else {
      for (String value : values) {
        addValue(name, value);
      }
    }
  }

  public void removeValue(String name) {
    for (Attribute param : parameters) {
      if (param.name.equals(name)) {
        parameters.remove(param);
        break;
      }
    }
  }

  //---

  public  ArrayList<Attribute> parameters;
}

