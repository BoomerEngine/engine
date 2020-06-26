package rtti;

import com.google.common.base.Stopwatch;
import utils.*;

import java.nio.charset.Charset;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.util.ArrayList;
import java.util.Collections;
import java.util.List;
import java.util.stream.Collectors;

public class DocGenerator {

    private Path projectPath;
    private Path outputPath;
    private String projectName;
    private boolean includeTests;

    public int numTypes = 0;
    public int numFiles = 0;

    public DocGenerator(Path _projectPath, Path _outputPath, String _projectName, boolean _includeTests) {
        this.projectPath = _projectPath;
        this.outputPath = _outputPath;
        this.projectName = _projectName;
        this.includeTests = _includeTests;
    }
    
    public void processAndGenerateReflection(GeneratedFilesCollection outputFiles)
    {
        // collect SOURCE files in the project
        List<Path> sourceFiles = collectSourceFiles(projectPath, includeTests);
        //System.out.printf("RTTI: Found %d source files in project '%s'\n", sourceFiles.size(), projectName);

        // process the source files and generate the
        List<DocNode> rootNodes = Collections.synchronizedList(new ArrayList<DocNode>());
        sourceFiles.stream().forEach(file -> processSourceFile(file, rootNodes));

        // sort root nodes by the source file name path, this is making sure the generator is deterministic
        rootNodes.sort((a,b) -> a.token.path.toString().compareTo(b.token.path.toString()));

        List<DocNode> allTypes = rootNodes.stream().flatMap(n -> n.stream()).collect(Collectors.toList());

        // extract types (classes and such)
        List<DocNode> types = rootNodes.stream()
                .flatMap(n -> n.stream())
                .filter(n -> (n.type == DocNodeType.CLASS) || (n.type == DocNodeType.CUSTOM_TYPE) || (n.type == DocNodeType.ENUM) || (n.type == DocNodeType.BITFIELD) || (n.type == DocNodeType.TESTHEADER) || (n.type == DocNodeType.GLOBAL_FUNC))
                .collect(Collectors.toList());
        //System.out.printf("RTTI: Found %d type definitions in project '%s'\n", types.size(), projectName);

        numTypes = types.size();
        numFiles = sourceFiles.size();

        // generate the rtti initialization code, we output directly to the file specified
        GeneratedFile f = outputFiles.createFile(outputPath);
        generateTypeInitializators(projectName, types, f, includeTests);
    }

    //---

    private static void generateForwardDeclaration(DocNode t, GeneratedFile f) {
        String platformName = t.root().data.value("platform");

        if (platformName != "")
            f.writelnf("#ifdef PLATFORM_" + platformName.toUpperCase());

        t.revparents().forEach(p -> f.writelnf("namespace %s { ", p.name));

        if (t.type == DocNodeType.TESTHEADER) {
            f.writelnf("extern void InitTests_%s(); ", t.name);
        } else if (t.type == DocNodeType.GLOBAL_FUNC) {
            f.writelnf("extern void RegisterGlobalFunc_%s(); ", t.name);
        } else {
            f.writelnf("extern void CreateType_%s(const char* name); ", t.name);
            f.writelnf("extern void InitType_%s();", t.name);
        }

        t.parents().forEach(p -> f.writeln("} "));

        if (platformName != "")
            f.writelnf("#endif");
    }

    private static void generateTypeInitCall(DocNode t, GeneratedFile f) {
        String platformName = t.root().data.value("platform");

        if (platformName != "")
            f.writelnf("#ifdef PLATFORM_" + platformName.toUpperCase());

        String namespacePath = t.revparents().map(p -> p.name).reduce("", (a,b) -> (a.isEmpty() ? b : a + "::" + b));
        if (t.type == DocNodeType.GLOBAL_FUNC) {
            f.writelnf("%s::RegisterGlobalFunc_%s();", namespacePath, t.name);
        } else {
            f.writelnf("%s::InitType_%s();", namespacePath, t.name);
        }

        if (platformName != "")
            f.writelnf("#endif");
    }

    private static void generateTestInitCall(DocNode t, GeneratedFile f) {
        String platformName = t.root().data.value("platform");

        if (platformName != "")
            f.writelnf("#ifdef PLATFORM_" + platformName.toUpperCase());

        String namespacePath = t.revparents().map(p -> p.name).reduce("", (a,b) -> (a.isEmpty() ? b : a + "::" + b));
        f.writelnf("%s::InitTests_%s();", namespacePath, t.name );

        if (platformName != "")
            f.writelnf("#endif");
    }

    private static void generateTypeCreateCall(DocNode t, GeneratedFile f) {
        String platformName = t.root().data.value("platform");

        // no init required
        if (t.type ==  DocNodeType.TESTHEADER || t.type == DocNodeType.GLOBAL_FUNC)
            return;

        if (platformName != "")
            f.writelnf("#ifdef PLATFORM_" + platformName.toUpperCase());

        String namespacePath = t.revparents().map(p -> p.name).reduce("", (a,b) -> (a.isEmpty() ? b : a + "::" + b));
        f.writelnf("%s::CreateType_%s(\"%s::%s\");", namespacePath, t.name, namespacePath, t.name );

        if (platformName != "")
            f.writelnf("#endif");
    }

    private static class InitElement
    {
        public DocNode node;
        public int priority;

        public InitElement(DocNode node_)
        {
            node = node_;
            priority = 0;

            if (node.type == DocNodeType.GLOBAL_FUNC)
                priority = 10;
            else if (node.type == DocNodeType.ENUM)
                priority = 1;
            else if (node.type == DocNodeType.BITFIELD)
                priority = 1;
            else if (node.type == DocNodeType.CUSTOM_TYPE)
                priority = 2;
            else if (node.type == DocNodeType.CLASS) {
                if (node.name.startsWith("I"))
                    priority = 2;
                if (node.name.toLowerCase().contains("metadata") && !node.name.toLowerCase().equals("metadata"))
                    priority = 3;
                else
                    priority = 4;
            }
        }
    };

    private static void generateTypeInitializators(String projectName, List<DocNode> types, GeneratedFile f, boolean includeTests) {
        /// header
        f.writeln( "/// Boomer Engine v4 by Tomasz \"RexDex\" Jonarski");
        f.writeln( "/// RTTI Glue Code Generator is under MIT License");
        f.writeln( "");
        f.writeln( "/// AUTOGENERATED FILE - ALL EDITS WILL BE LOST");

        // separator
        f.writeln("");
        f.writeln( "// --------------------------------------------------------------------------------");
        f.writeln( "");

        // output forward declarations
        for (DocNode t : types)
            generateForwardDeclaration(t, f);

        // separator
        f.writeln( "");
        f.writeln( "// --------------------------------------------------------------------------------");
        f.writeln( "");

        // builder function
        f.writelnf("void InitializeReflection_%s()", projectName);
        f.writelnf("{");

        // call the creation functions
        for (DocNode t : types)
            generateTypeCreateCall(t, f);

        // prepare ordered list of initializers to call
        List<InitElement> initList = new ArrayList<>();
        for (DocNode t : types) {
            if (t.type == DocNodeType.TESTHEADER) continue;
            initList.add(new InitElement(t));
        }
        initList.sort((a, b) -> a.node.name.compareTo(b.node.name));
        initList.sort((a, b) -> Integer.compare(a.priority, b.priority));

        // call the initialize functions in order
        for (InitElement elem : initList)
            generateTypeInitCall(elem.node, f);

        // end of function
        f.writelnf("}");

        // separator
        f.writeln("");
        f.writeln( "// --------------------------------------------------------------------------------");
        f.writeln( "");

        // builder function
        f.writelnf("void InitializeTests_%s()", projectName);
        f.writelnf("{");

        // call the text initialize functions
        if (includeTests) {
            types.stream()
                    .filter(t -> t.type == DocNodeType.TESTHEADER)
                    .forEach(t -> generateTestInitCall(t, f));
        }

        // end of function
        f.writelnf("}");

        // separator
        f.writeln("");
        f.writeln( "// --------------------------------------------------------------------------------");
        f.writeln( "");

        f.writelnf("");
        f.writelnf("#ifdef DECLARE_MODULE");
        f.writelnf("#undef DECLARE_MODULE");
        f.writelnf("#endif");
        f.writelnf("#define DECLARE_MODULE(_projectName) DECLARE_MODULE_WITH_REFLECTION_IMPL(_projectName)");
    }

    //---

    private static void processSourceFile(Path absolutePath, List<DocNode> documentNodes) {
        try {
            // load file
            String content = utils.Utils.loadFileToString(absolutePath, Charset.defaultCharset());

            // load the settings
            String filePreamble = content.substring(0, Math.min(2048, content.length()));
            KeyValueTable attributes = KeyValueTable.parseFromParamBlock(filePreamble);

            // get the platform name
            String platformName = attributes.value("platform");

            // tokenize
            List<CodeToken> tokens = CodeTokenizer.tokenize(content);
            if (!tokens.isEmpty()) {

                // attach to file (for context in case of errors)
                tokens.forEach(t -> t.path = absolutePath);

                // extract types
                DocNode rootNode = new DocNode(DocNodeType.FILE, tokens.get(0), null, "");
                rootNode.data.setValue("platform", platformName);
                DocAnalyzer.anylyze(new CodeTokenStream(tokens), rootNode);
                documentNodes.add(rootNode);
            }
        } catch (Exception e) {
            throw new ProjectException("Failed to process source file", e).location(absolutePath);
        }
    }

    private static void collectSourceFiles(Path absolutePath, List<Path> outputList, boolean includeTests) {
        utils.Utils.getFilePaths(absolutePath)
                .stream()
                .filter(p -> p.toString().endsWith(".cpp"))
                .forEach(childPath -> {
                    // ignore test files
                    if (!includeTests && childPath.toString().endsWith("_test.cpp")) {
                        System.out.printf("File %s skipped because it's a test file\n", childPath);
                        return;
                    }
                    outputList.add(childPath);
                });

        utils.Utils.getSubDirPaths(absolutePath)
                .stream().forEach(childPath -> collectSourceFiles(childPath, outputList, includeTests));
    }

    private static List<Path> collectSourceFiles(Path projectPath, boolean includeTests) {
        List<Path> files = Collections.synchronizedList(new ArrayList<>());
        collectSourceFiles(projectPath, files, includeTests);
        return files;
    }
}
