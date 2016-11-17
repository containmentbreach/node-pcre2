Description
===========

A pcre2 binding for [node.js](http://nodejs.org/) with UTF8 and Unicode properties support.
Same API as RegExp class! But with bunch of new features like negative/positive lookbehind.


Requirements
============

* [node.js](http://nodejs.org/) -- v0.8.0 or newer
* Windows, Linux, or OSX


Install
=======

    npm install pcre2

	
USAGE
=====

...
var PCRE2 = require("pcre2").PCRE2;
var PCRE2JIT = require("pcre2").PCRE2JIT; //better performance, longer regex compiling, usually you dont need this class instead of first one.

var re = new PCRE2("^(abc|123)+$", "gi");
var output = re.exec("abc123abc123");

//output is:
//Array[2]:
//   0:"abc123abc123"
//   1:"123"
//   index:0
//   input:"abc123abc123"
...

API
===

Same as v8's RegExp
Plus extra modifiers and syntax, see: https://github.com/jpcre2/jpcre2
