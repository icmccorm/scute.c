# Scute.c
A C interpreter for the Scute programming language.
Check out the live development [build](https://ian-colcanon.github.io/scute/). 

## What is Scute?
Scute is a scripting language designed for producing graphics and animations in the SVG file format. Popular image editors like Adobe Illustrator and Inkscape are difficult to use for creating mathematically-based graphics or highly repetitive designs. Scute solves this problem by allowing complex shapes to be created programmatically while retaining the useful drag-and-drop features of image editors. 

## How does this work?
To accomplish this, the Scute language includes a split-pane visual editor, with a text editor area and a viewing area. Users can write-up a program for a shape in the text editor, and then adjust it by dragging and dropping visual handles in the viewing area. Using a trick called trace-based program synthesis, any changes made in the viewing area automatically update the corresponding source code. For example, dragging a rectangle from the point (0,0) to (100, 100) will change the values in the Scute source code for the rectangle where its position is defined.

## What's under the hood?
The Scute interpreter is written in C with the help of a useful guide by Bob Nystrom titled [Crafting Interpreters](https://www.craftinginterpreters.com/). For Scute to run in the browser, it is transpiled from C to WebAssembly using [Emscripten](https://emscripten.org/).

The visual IDE for Scute is a React JS web application written in TypeScript. The Scute interpreter is run within a WebWorker, and communicates asynchronously with the IDE to compile and run programs. 

A full repository containing the C code for scute is located [here](https://github.com/ian-colcanon/scute/edit/master/README.md). 

## Why does it exist?
Scute is part of a research project in domain-specific programming languages, and is one of many designed for creating scalable graphics and animations. Scute is unique because it explores the area of direct manipulation, building from the work of Ravi Chugh and his fellow researchers at the University of Chicago. The technique of trace-based program synthesis is outlined in Chugh's [2016 paper](https://arxiv.org/pdf/1507.02988.pdf) "Programmatic and Direct Manipulation, Together at Last," which features its implementation in the [Sketch-N-Sketch](https://github.com/ravichugh/sketch-n-sketch) programming language.

## References
*	Chugh, Ravi et al. “Programmatic and Direct Manipulation, Together at Last.” Proceedings of the 37th ACM SIGPLAN Conference 		on Programming Language Design and Implementation - PLDI 2016 (2016). arXiv. DOI=https://arxiv.org/abs/1507.02988
*	Johnson, Gabe. 2008. FlatCAD and FlatLang: Kits by Code. In Proceedings of the 2008 IEEE Symposium on Visual Languages and Human-Centric Computing (VL/HCC) (Herrsching, Germany, 2008). IEEE, Piscataway, NJ. DOI=10.1109/VLHCC.2008.4639070
*	Nystrom, Robert. Crafting Interpreters. Retrieved from https://www.craftinginterpreters.com/.
*	Turbak, F. et al. Blocks Languages for Creating Tangible Artifacts. In Proceedings of the 2012 IEEE Symposium on Visual 		Languages and Human-Centric Computing (Innsbruck, Austria, Sept. 30 - Oct 04, 2012). IEEE, Piscataway, NJ. 			DOI=10.1109/VLHCC.2012.6344500
