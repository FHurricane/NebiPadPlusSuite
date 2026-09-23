#include "language.h"

#include <QCollator>
#include <QFileInfo>
#include <QRegularExpression>

#include <algorithm>

const QVector<LanguageDefinition> &languages()
{
    static const QVector<LanguageDefinition> catalog = [] {
        QVector<LanguageDefinition> result;
        const auto add = [&result](const char *name, const char *extensions,
                                   const char *shebang, const char *lineComment,
                                   const char *keywords, IndentStyle style,
                                   int width = 4, bool tabs = false,
                                   const char *blockStart = "", const char *blockEnd = "") {
            result.push_back({name, QString::fromLatin1(extensions).split(' ', Qt::SkipEmptyParts),
                              shebang, lineComment, blockStart, blockEnd,
                              QString::fromLatin1(keywords).split(' ', Qt::SkipEmptyParts),
                              style, width, tabs});
        };
        add("Ada", "adb ads", "", "--", "begin end is procedure function package if then else loop while for return type with use", IndentStyle::Keywords);
        add("Assembly", "asm s", "", ";", "section global extern mov add sub cmp jmp call ret push pop", IndentStyle::Preserve);
        add("Arduino Files", "ino", "", "//", "setup loop pinMode digitalWrite digitalRead analogRead analogWrite delay millis micros HIGH LOW INPUT OUTPUT INPUT_PULLUP Serial", IndentStyle::Braces, 4, false, "/*", "*/");
        add("AWK", "awk", "#!/usr/bin/env -S awk -f", "#", "BEGIN END if else while for function return print printf next", IndentStyle::Braces);
        add("Bash", "bash", "#!/usr/bin/env bash", "#", "if then else elif fi for while do done case esac function in", IndentStyle::Keywords);
        add("C", "c", "", "//", "if else for while do switch case struct enum typedef return const static void int char", IndentStyle::Braces, 4, false, "/*", "*/");
        add("C#", "cs", "", "//", "class public private protected internal static void int string var if else for foreach while return using namespace", IndentStyle::Braces, 4, false, "/*", "*/");
        add("C++", "cpp cc cxx h hpp hh hxx", "", "//", "class struct namespace template typename auto const constexpr if else for while return public private protected", IndentStyle::Braces, 4, false, "/*", "*/");
        add("Clojure", "clj cljs cljc edn", "", ";", "def defn let fn if do cond for doseq ns require true false nil", IndentStyle::Paren, 2);
        add("COBOL", "cob cbl", "", "*>", "IDENTIFICATION DIVISION PROGRAM-ID DATA PROCEDURE SECTION IF ELSE END-IF PERFORM MOVE TO", IndentStyle::Keywords);
        add("D", "d", "", "//", "module import class struct if else for foreach while return auto immutable", IndentStyle::Braces, 4, false, "/*", "*/");
        add("Dart", "dart", "#!/usr/bin/env dart", "//", "class void var final const if else for while return import async await Future", IndentStyle::Braces, 2, false, "/*", "*/");
        add("Elixir", "ex exs", "#!/usr/bin/env elixir", "#", "defmodule def defp do end if else case cond fn import alias use", IndentStyle::Keywords, 2);
        add("Erlang", "erl hrl", "", "%", "module export import case of end fun if receive after try catch", IndentStyle::Keywords);
        add("F#", "fs fsx fsi", "#!/usr/bin/env -S dotnet fsi", "//", "let module open type match with if then else function member", IndentStyle::Keywords, 4, false, "(*", "*)");
        add("Fortran", "f f90 f95 f03 f08", "", "!", "program module subroutine function end if then else do select case implicit none", IndentStyle::Keywords);
        add("Go", "go", "", "//", "package import func type struct interface if else for range return go defer", IndentStyle::Braces, 4, true, "/*", "*/");
        add("Groovy", "groovy gvy", "#!/usr/bin/env groovy", "//", "def class import if else for while return closure as in", IndentStyle::Braces, 4, false, "/*", "*/");
        add("Haskell", "hs lhs", "#!/usr/bin/env runhaskell", "--", "module import where let in data type class instance if then else case of do", IndentStyle::Keywords);
        add("Java", "java", "", "//", "class interface public private protected static void int String if else for while return new import package", IndentStyle::Braces, 4, false, "/*", "*/");
        add("JavaScript", "js mjs cjs jsx", "#!/usr/bin/env node", "//", "const let var function class if else for while return async await import export new", IndentStyle::Braces, 2, false, "/*", "*/");
        add("Julia", "jl", "#!/usr/bin/env julia", "#", "function end if else elseif for while begin let struct mutable return using import", IndentStyle::Keywords);
        add("Kotlin", "kt kts", "#!/usr/bin/env kotlin", "//", "fun val var class object if else when for while return import package suspend", IndentStyle::Braces, 4, false, "/*", "*/");
        add("Lua", "lua", "#!/usr/bin/env lua", "--", "function local if then else elseif end for while do repeat until return", IndentStyle::Keywords, 4, false, "--[[", "]]");
        add("Makefile", "mk makefile", "", "#", "include define endef ifeq ifneq endif .PHONY", IndentStyle::Preserve, 4, true);
        add("MATLAB", "matlab", "", "%", "function end if else elseif for while switch case return classdef", IndentStyle::Keywords);
        add("Nim", "nim", "", "#", "proc func method type object var let const if elif else for while return import", IndentStyle::Colon, 2);
        add("Objective-C", "m mm", "", "//", "interface implementation end property synthesize if else for while return self super", IndentStyle::Braces, 4, false, "/*", "*/");
        add("OCaml", "ml mli", "", "", "let rec in match with type module open fun if then else begin end", IndentStyle::Keywords, 2, false, "(*", "*)");
        add("Pascal", "pas pp", "", "//", "program unit uses begin end var const type procedure function if then else for while do", IndentStyle::Keywords, 2, false, "{", "}");
        add("Perl", "pl pm", "#!/usr/bin/env perl", "#", "my our sub if else elsif for foreach while return use package", IndentStyle::Braces);
        add("PHP", "php phtml", "#!/usr/bin/env php", "//", "function class public private protected if else foreach while return echo namespace use", IndentStyle::Braces, 4, false, "/*", "*/");
        add("PowerShell", "ps1 psm1", "#!/usr/bin/env pwsh", "#", "function param if else elseif foreach while return switch class", IndentStyle::Braces);
        add("Python", "py pyw", "#!/usr/bin/env python3", "#", "def class if elif else for while try except finally with return import from async await", IndentStyle::Colon);
        add("R", "r", "#!/usr/bin/env Rscript", "#", "function if else for while repeat in next break library require", IndentStyle::Braces, 2);
        add("Ruby", "rb rake", "#!/usr/bin/env ruby", "#", "def class module if elsif else unless while until do end return require", IndentStyle::Keywords, 2);
        add("Rust", "rs", "", "//", "fn let mut struct enum impl trait pub use mod if else for while match return", IndentStyle::Braces, 4, false, "/*", "*/");
        add("Scala", "scala sc", "#!/usr/bin/env scala", "//", "object class trait def val var if else for while match case import", IndentStyle::Braces, 2, false, "/*", "*/");
        add("Shell", "sh", "#!/usr/bin/env sh", "#", "if then else elif fi for while do done case esac in export", IndentStyle::Keywords);
        add("SQL", "sql", "", "--", "SELECT FROM WHERE INSERT INTO UPDATE DELETE CREATE TABLE JOIN ON GROUP BY ORDER HAVING AS", IndentStyle::Keywords, 4, false, "/*", "*/");
        add("Swift", "swift", "#!/usr/bin/env swift", "//", "func let var class struct enum protocol if else for while return import guard", IndentStyle::Braces, 4, false, "/*", "*/");
        add("Tcl", "tcl", "#!/usr/bin/env tclsh", "#", "proc if else elseif foreach while for return set namespace variable", IndentStyle::Braces);
        add("TypeScript", "ts tsx", "", "//", "const let var function class interface type if else for while return async await import export", IndentStyle::Braces, 2, false, "/*", "*/");
        add("Visual Basic", "vb vbs", "", "'", "Dim Sub Function End If Then Else For Each Next While Select Case Return Class", IndentStyle::Keywords);
        add("Zig", "zig", "", "//", "const var pub fn if else while for return struct enum union import", IndentStyle::Braces, 4);
        add("Zsh", "zsh", "#!/usr/bin/env zsh", "#", "if then else elif fi for while do done function local typeset", IndentStyle::Keywords);
        add("ActionScript", "as", "", "//", "class function var const if else for while return package import public private", IndentStyle::Braces, 4, false, "/*", "*/");
        add("ASP", "asp aspx", "", "'", "Sub Function End If Then Else For Each Next While Response Request Server", IndentStyle::Keywords);
        add("AutoIt", "au3", "", ";", "Func EndFunc If Then Else ElseIf EndIf For Next While WEnd Return Local Global", IndentStyle::Keywords);
        add("AviSynth", "avs avsi", "", "#", "function return import global if else true false", IndentStyle::Braces, 4, false, "/*", "*/");
        add("Batch", "bat cmd", "", "::", "echo set if else for goto call exit rem pushd popd", IndentStyle::Keywords);
        add("CoffeeScript", "coffee litcoffee", "#!/usr/bin/env coffee", "#", "class extends if else for while return do unless then and or not", IndentStyle::Colon, 2);
        add("Crystal", "cr", "#!/usr/bin/env -S crystal run", "#", "class module def end if else elsif unless while until do return require", IndentStyle::Keywords, 2);
        add("CSS", "css scss less", "", "//", "color background display position margin padding font border width height flex grid", IndentStyle::Braces, 2, false, "/*", "*/");
        add("Elm", "elm", "", "--", "module exposing import type alias let in case of if then else", IndentStyle::Keywords, 4, false, "{-", "-}");
        add("Fish", "fish", "#!/usr/bin/env fish", "#", "function end if else for while switch case set return", IndentStyle::Keywords);
        add("Forth", "fth 4th", "#!/usr/bin/env gforth", "\\", ": ; if else then begin until while repeat variable constant", IndentStyle::Preserve);
        add("GDScript", "gd", "", "#", "extends class_name func var const if elif else for while return signal", IndentStyle::Colon, 4);
        add("GLSL", "glsl vert frag geom comp", "", "//", "uniform in out void vec2 vec3 vec4 mat4 float int if else for return layout", IndentStyle::Braces, 4, false, "/*", "*/");
        add("Haxe", "hx", "", "//", "class interface function var if else for while return package import", IndentStyle::Braces, 4, false, "/*", "*/");
        add("HTML", "html htm", "", "", "html head body div span script style title meta link p a ul li table", IndentStyle::Markup, 2, false, "<!--", "-->");
        add("JSON", "json jsonc", "", "//", "true false null", IndentStyle::Braces, 2, false, "/*", "*/");
        add("Lisp", "lisp lsp cl", "#!/usr/bin/env -S sbcl --script", ";", "defun defvar defparameter let if cond lambda progn loop format setq", IndentStyle::Paren, 2);
        add("Markdown", "md markdown", "", "", "", IndentStyle::Preserve, 2);
        add("Nix", "nix", "", "#", "let in with rec inherit if then else import builtins", IndentStyle::Braces, 2, false, "/*", "*/");
        add("Octave", "octave", "#!/usr/bin/env -S octave -qf", "%", "function endfunction if endif else elseif for endfor while endwhile", IndentStyle::Keywords);
        add("Prolog", "pro", "#!/usr/bin/env -S swipl -s", "%", "is not true false fail assert retract consult dynamic", IndentStyle::Preserve, 4, false, "/*", "*/");
        add("PureScript", "purs", "", "--", "module import where data type class instance let in do case of", IndentStyle::Keywords, 2, false, "{-", "-}");
        add("Scheme", "scm ss", "#!/usr/bin/env -S guile -s", ";", "define lambda let if cond begin set! quote import", IndentStyle::Paren, 2);
        add("Solidity", "sol", "", "//", "pragma contract function public private external view pure returns if else for while", IndentStyle::Braces, 4, false, "/*", "*/");
        add("SVG", "svg", "", "", "svg path rect circle line polygon g defs", IndentStyle::Markup, 2, false, "<!--", "-->");
        add("Verilog", "v sv svh", "", "//", "module endmodule input output wire reg always begin end if else assign", IndentStyle::Keywords, 4, false, "/*", "*/");
        add("VHDL", "vhd vhdl", "", "--", "entity architecture begin end process signal variable if then else", IndentStyle::Keywords);
        add("WebAssembly", "wat wast", "", ";;", "module func param result local export import memory data", IndentStyle::Preserve, 2, false, "(;", ";)");
        add("XML", "xml xsd xsl xslt", "", "", "xml version encoding", IndentStyle::Markup, 2, false, "<!--", "-->");
        add("YAML", "yaml yml", "", "#", "true false null yes no", IndentStyle::Preserve, 2);
        add("ASN.1", "asn asn1", "", "--", "DEFINITIONS BEGIN END IMPORTS EXPORTS SEQUENCE SET CHOICE INTEGER BOOLEAN", IndentStyle::Keywords);
        add("BaanC", "bc baan", "", "|", "function extern domain table if then else endif for while return", IndentStyle::Keywords);
        add("BlitzBasic", "bb blitz", "", ";", "Function End Function Type End Type If Then Else For Next While Wend Return", IndentStyle::Keywords);
        add("CMake", "cmake", "", "#", "function endfunction macro endmacro if elseif else endif foreach endforeach set project add_executable", IndentStyle::Keywords, 2);
        add("Csound", "csd orc sco", "", ";", "instr endin opcode endop if then else endif while do od", IndentStyle::Keywords);
        add("Diff", "diff patch", "", "", "", IndentStyle::Preserve);
        add("Escript", "escript", "#!/usr/bin/env escript", "%", "module export main case of end fun if receive", IndentStyle::Keywords);
        add("Fortran fixed form", "f77 for", "", "!", "PROGRAM SUBROUTINE FUNCTION END IF THEN ELSE DO CONTINUE", IndentStyle::Keywords);
        add("FreeBasic", "bas bi", "", "'", "Function End Sub If Then Else For Next While Wend Return Dim", IndentStyle::Keywords);
        add("Gui4Cli", "gui4cli", "", ";", "Window EndWindow Button Edit Menu OnClick", IndentStyle::Keywords);
        add("Hollywood", "hws", "", ";", "Function EndFunction If Else For While Return", IndentStyle::Keywords);
        add("INI", "ini cfg", "", ";", "", IndentStyle::Preserve);
        add("Inno Setup", "iss", "", ";", "Setup Files Icons Run Code if then else begin end", IndentStyle::Keywords);
        add("Intel HEX", "hex ihex", "", "", "", IndentStyle::Preserve);
        add("JSON5", "json5", "", "//", "true false null Infinity NaN", IndentStyle::Braces, 2, false, "/*", "*/");
        add("JSP", "jsp", "", "", "page import include if else for while class", IndentStyle::Markup, 2, false, "<!--", "-->");
        add("KiXtart", "kix", "", ";", "IF ELSE ENDIF FOR EACH NEXT WHILE LOOP FUNCTION ENDFUNCTION", IndentStyle::Keywords);
        add("LaTeX", "latex sty cls", "", "%", "begin end document class usepackage section subsection", IndentStyle::Preserve, 2);
        add("MMIXAL", "mms", "", "%", "LOC IS BYTE WYDE TETRA OCTA JMP GO PUSHJ POP", IndentStyle::Preserve);
        add("MSSQL", "mssql", "", "--", "SELECT FROM WHERE INSERT INTO UPDATE DELETE CREATE TABLE JOIN EXEC DECLARE BEGIN END", IndentStyle::Keywords, 4, false, "/*", "*/");
        add("Nncrontab", "tab nncron", "", "#", "TIME ACTION FILE WATCH", IndentStyle::Preserve);
        add("NSIS", "nsi nsh", "", ";", "Section SectionEnd Function FunctionEnd IfFileExists StrCmp Call Return", IndentStyle::Keywords);
        add("OScript", "oscript", "", "//", "function end if else for while return", IndentStyle::Keywords, 4, false, "/*", "*/");
        add("PostScript", "ps eps", "", "%", "def if ifelse for repeat loop exit moveto lineto stroke show", IndentStyle::Preserve);
        add("Properties", "properties", "", "#", "", IndentStyle::Preserve);
        add("PureBasic", "pb pbi", "", ";", "Procedure EndProcedure If Else EndIf For Next While Wend Define Global", IndentStyle::Keywords);
        add("Raku", "raku rakumod p6", "#!/usr/bin/env raku", "#", "sub method class role my our if else for loop while return use", IndentStyle::Braces, 4, false, "#`[", "]");
        add("Rebol", "reb r3", "#!/usr/bin/env rebol", ";", "REBOL func function if either foreach repeat while return", IndentStyle::Keywords);
        add("Registry", "reg", "", ";", "Windows Registry Editor Version", IndentStyle::Preserve);
        add("Resource Script", "rc rc2", "", "//", "BEGIN END DIALOG MENU STRINGTABLE ICON BITMAP", IndentStyle::Keywords, 4, false, "/*", "*/");
        add("S-Record", "srec s19 s28 s37", "", "", "", IndentStyle::Preserve);
        add("SAS", "sas", "", "*", "DATA PROC RUN QUIT SET MERGE IF THEN ELSE DO END", IndentStyle::Keywords, 4, false, "/*", "*/");
        add("Smalltalk", "st", "", "", "self super nil true false ifTrue ifFalse whileTrue do", IndentStyle::Preserve, 2, false, "\"", "\"");
        add("SPICE", "cir spice sp", "", "*", ".model .subckt .ends .tran .ac .dc .param", IndentStyle::Preserve);
        add("Tektronix HEX", "tek tehex", "", "", "", IndentStyle::Preserve);
        add("TeX", "tex", "", "%", "begin end def input include catcode", IndentStyle::Preserve, 2);
        add("TOML", "toml", "", "#", "true false", IndentStyle::Preserve, 2);
        add("txt2tags", "t2t", "", "%", "", IndentStyle::Preserve);
        add("Visual Prolog", "prolog vip", "", "%", "predicates clauses goal domains facts class interface implement", IndentStyle::Keywords);
        QCollator collator;
        collator.setCaseSensitivity(Qt::CaseInsensitive);
        std::sort(result.begin(), result.end(), [&collator](const auto &a, const auto &b) {
            return collator.compare(a.name, b.name) < 0;
        });
        return result;
    }();
    return catalog;
}

int detectLanguage(const QString &path, const QString &text)
{
    const QString firstLine = text.section('\n', 0, 0).trimmed();
    if (firstLine.startsWith("#!")) {
        const auto commandFor = [](const QString &shebang) {
            const QStringList parts = shebang.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
            QString command = parts.value(0).section('/', -1);
            if (command == "env") {
                int index = 1;
                if (parts.value(index) == "-S") ++index;
                command = parts.value(index).section('/', -1);
            }
            return command;
        };
        QString command = commandFor(firstLine);
        if (command.startsWith("python")) command = "python3";
        if (command == "nodejs") command = "node";
        for (int i = 0; i < languages().size(); ++i) {
            const QString shebangCommand = commandFor(languages()[i].shebang);
            if (!shebangCommand.isEmpty() && command.compare(shebangCommand, Qt::CaseInsensitive) == 0)
                return i;
        }
    }
    const QString suffix = QFileInfo(path).suffix().toLower();
    const QString fileName = QFileInfo(path).fileName().toLower();
    for (int i = 0; i < languages().size(); ++i) {
        if (languages()[i].extensions.contains(suffix) ||
            (languages()[i].name == "Makefile" && fileName == "makefile")) return i;
    }
    return -1;
}
