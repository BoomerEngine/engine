package rtti;

import utils.CodeToken;
import utils.KeyValueTable;

import java.util.ArrayList;
import java.util.List;
import java.util.stream.Stream;

public class DocNode {

  //--

  public String name = "";
  public final DocNodeType type;
  public final CodeToken token; // original token, used as a reference to the position, also contains the name
  public final KeyValueTable data = new KeyValueTable(); // collected data, depends on the node

  public final DocNode parent; // parent node
  public List<DocNode> children = new ArrayList<DocNode>();

  public DocNode root() { return (parent != null) ? parent.root() : this; }


  //---

  public DocNode(DocNodeType type, CodeToken token, DocNode parent, String name) {
    this.type = type;
    this.token = token;
    this.parent = parent;
    this.name = name;
  }

  public DocNode createChild(DocNodeType type, CodeToken token, String name) {
    DocNode ret = new DocNode(type, token, this, name);
    children.add(ret);
    return ret;
  }

  public Stream<DocNode> stream() {
    return Stream.concat( Stream.of(this), children.stream().flatMap(x -> x.stream()) );
  }

  public Stream<DocNode> parents() {
    if (parent == null || parent.type != DocNodeType.NAMESPACE)
      return Stream.empty();

    return Stream.concat(Stream.of(parent), parent.parents());
  }

  public Stream<DocNode> revparents() {
    if (parent == null || parent.type != DocNodeType.NAMESPACE)
      return Stream.empty();

    return Stream.concat(parent.revparents(), Stream.of(parent));
  }

  //---

}
