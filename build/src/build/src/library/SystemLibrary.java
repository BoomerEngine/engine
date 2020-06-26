package library;

import org.w3c.dom.Element;
import utils.KeyValueTable;

/// system library is a library that can be discovered on the system without the need to include it in the dependencies
public class SystemLibrary {

    //---

    // get name to lookup in the system
    public String getName() { return name; }

    // get the variable name that will contain the include paths for the library
    public String getIncludeDirsNames() { return includeDirsNames; }

    // get the variable name that will contain the link directories
    public String getLinkDirsNames() { return linkDirsNames; };

    // get the variable name that will contain the libraries to include
    public String getLibraryFiles() { return libraryFiles; }

    // get the custom definitions
    public String getDefinitionsName() { return definitionsName; }

    // additional include fle
    public String getIncludeFile() { return includeFile; }

    // get additional compiler flags
    public String getFlags() { return flags; }

    public SystemLibrary(Element xmlElement) {
        // the selector is created from the node attribute
        KeyValueTable params = utils.Utils.getXmlNodeAttributes(xmlElement);
        this.name = params.value("name");
        System.out.println("Found CMAKE library setup for '" + this.name + "'");

        // get the variable with library includes
        this.includeDirsNames = params.valueOrDefault("includeDirs", this.name.toUpperCase() + "_INCLUDE_DIR");
        this.linkDirsNames = params.valueOrDefault("linkDirs", this.name.toUpperCase() + "_LINK_DIR");
        this.libraryFiles = params.valueOrDefault("libraries", this.name.toUpperCase() + "_LIBRARIES");
        this.definitionsName = params.value("definitions");
        this.includeFile = params.value("includeFile");
        this.flags = params.value("flags");
    }

    //----

    private String name;
    private String includeDirsNames;
    private String linkDirsNames;
    private String libraryFiles;
    private String includeFile;
    private String definitionsName;
    private String flags;
}
