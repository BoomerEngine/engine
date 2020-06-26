package rtti;

import utils.CodeToken;
import utils.CodeTokenStream;
import utils.CodeTokenType;
import utils.ProjectException;

public class DocAnalyzer {

  public static void anylyze(CodeTokenStream tokens, DocNode rootNode) {
    while (tokens.hasContent()) {
      scanNamespace(tokens, rootNode);
    }
  }

  private static String extractMacroParam(CodeTokenStream tokens)
  {
    if (!tokens.eat().toString().equals("("))
      throw new ProjectException("Expected '('").location(tokens.consumed().path, tokens.consumed().line());

    if (tokens.eat().type() != CodeTokenType.Token)
      throw new ProjectException("Expected identifier name").location(tokens.consumed().path, tokens.consumed().line());

    String ret = tokens.consumed().toString();

    /*if (tokens.eat().toString().equals(")")) {
      if (!tokens.eat().toString().equals(";"))
        throw new ProjectException("Expected ';'").location(tokens.consumed().path, tokens.consumed().line());
    }
    else if (tokens.eat().toString().equals(",")) {
      // valid
    }
    else {
       throw new ProjectException("Expected ';' or ','").location(tokens.consumed().path, tokens.consumed().line());
    }*/

    return ret;
 }

  private static void scanNamespace(CodeTokenStream tokens, DocNode rootNode) {
    int bracketLevel = 0;

    // very simple parsing loop, look for the namespaces
    while (tokens.hasContent())
    {
      CodeToken cur = tokens.eat();
      String txt = cur.toString();

      // enter sub-namespace scope
      if (txt.equals("namespace")) {
        if (tokens.peekText(1).equals("{"))
        {
          String name = tokens.peekText(0);
          tokens.consume(2); // name and the bracket
          DocNode node = rootNode.createChild(DocNodeType.NAMESPACE, cur, name);
          scanNamespace(tokens, node);
        }
      }

      // scope tracing
      else if (txt.equals("{")) {
        bracketLevel += 1;
      }
      else if (txt.equals("}")) {
        if (bracketLevel == 0)
          break;
        bracketLevel -= 1;
      }

      // macros
      switch (txt) {
        case "RTTI_BEGIN_TYPE_CLASS":
        case "RTTI_BEGIN_TYPE_STRUCT":
        case "RTTI_BEGIN_TYPE_NATIVE_CLASS":
        case "RTTI_BEGIN_TYPE_ABSTRACT_CLASS": {
          String className = extractMacroParam(tokens);
          DocNode node = rootNode.createChild(DocNodeType.CLASS, cur, className);
          break;
        }

        case "RTTI_BEGIN_CUSTOM_TYPE":
        case "RTTI_BEGIN_CUSTOM_TYPE_NO_SERIALIZATION": {
          String className = extractMacroParam(tokens);
          DocNode node = rootNode.createChild(DocNodeType.CUSTOM_TYPE, cur, className);
          break;
        }

        case "RTTI_BEGIN_TYPE_ENUM": {
          String className = extractMacroParam(tokens);
          DocNode node = rootNode.createChild(DocNodeType.ENUM, cur, className);
          break;
        }

        case "RTTI_BEGIN_TYPE_BITFIELD": {
          String className = extractMacroParam(tokens);
          DocNode node = rootNode.createChild(DocNodeType.BITFIELD, cur, className);
          break;
        }

        case "DECLARE_TEST_FILE": {
          String fileName = extractMacroParam(tokens);
          DocNode node = rootNode.createChild(DocNodeType.TESTHEADER, cur, fileName);
          break;
        }

        case "RTTI_GLOBAL_FUNCTION":
        case "RTTI_GLOBAL_FUNCTION_EX": {
          String functionName = extractMacroParam(tokens);
          DocNode node = rootNode.createChild(DocNodeType.GLOBAL_FUNC, cur, functionName);
          break;
        }
      }
    }
  }

}
