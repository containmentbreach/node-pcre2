var lib = require('../build/Release/pcre2');

module.exports = {
	PCRE2: lib.PCRE2,
	PCRE2JIT: lib.PCRE2JIT
};

var StringMatch = String.prototype.match;
var StringReplace = String.prototype.replace;

String.prototype.match = function(regexp) {
	if ( regexp instanceof RegExp ) {
		return StringMatch.apply(this, arguments);
	}
	else {
		return regexp.match(this);
	}
}

String.prototype.replace = function(searchVal, replaceVal) {
	if ( typeof(searchVal) == "string" || searchVal instanceof RegExp ) {
		return StringReplace.apply(this, arguments);
	}
	else {
		return searchVal.replace(this, replaceVal);
	}
}