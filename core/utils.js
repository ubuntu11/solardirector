
//include(dirname(script_name)+"/underscore-min.js");

function arrayUnique(array) {
    var a = array.concat();
    for(var i=0; i<a.length; ++i) {
        for(var j=i+1; j<a.length; ++j) {
            if(a[i] === a[j])
                a.splice(j--, 1);
        }
    }

    return a;
}

Array.prototype.unique = function() {
    var a = this.concat();
    for(var i=0; i<a.length; ++i) {
        for(var j=i+1; j<a.length; ++j) {
            if(a[i] === a[j])
                a.splice(j--, 1);
        }
    }

    return a;
};

function merge(target, source) {
    Object.keys(source).forEach(function (key) {
        if (source[key] && typeof source[key] === 'object') {
            merge(target[key] = target[key] || {}, source[key]);
            return;
        }
        target[key] = source[key];
    });
}

var STRIP_COMMENTS = /((\/\/.*$)|(\/\*[\s\S]*?\*\/))/mg;
var ARGUMENT_NAMES = /([^\s,]+)/g;
function getParamNames(func) {
  var fnStr = func.toString().replace(STRIP_COMMENTS, '');
  var result = fnStr.slice(fnStr.indexOf('(')+1, fnStr.indexOf(')')).match(ARGUMENT_NAMES);
  if(result === null)
     result = [];
  return result;
}

function isInt(value) {
  var x;
  return isNaN(value) ? !1 : (x = parseFloat(value), (0 | x) === x);
}

function strstr(haystack, needle, bool) {
    // Finds first occurrence of a string within another
    //
    // version: 1103.1210
    // discuss at: http://phpjs.org/functions/strstr    // +   original by: Kevin van Zonneveld (http://kevin.vanzonneveld.net)
    // +   bugfixed by: Onno Marsman
    // +   improved by: Kevin van Zonneveld (http://kevin.vanzonneveld.net)
    // *     example 1: strstr(.Kevin van Zonneveld., .van.);
    // *     returns 1: .van Zonneveld.    // *     example 2: strstr(.Kevin van Zonneveld., .van., true);
    // *     returns 2: .Kevin .
    // *     example 3: strstr(.name@example.com., .@.);
    // *     returns 3: .@example.com.
    // *     example 4: strstr(.name@example.com., .@., true);    // *     returns 4: .name.
    var pos = 0;

    haystack += "";
    pos = haystack.indexOf(needle); if (pos == -1) {
        return false;
    } else {
        if (bool) {
            return haystack.substr(0, pos);
        } else {
            return haystack.slice(pos);
        }
    }
}

//const clone = JSON.parse(JSON.stringify(orig));
function clone(obj) {
    var copy;

    // Handle the 3 simple types, and null or undefined
    if (null == obj || "object" != typeof obj) return obj;

    // Handle Date
    if (obj instanceof Date) {
        copy = new Date();
        copy.setTime(obj.getTime());
        return copy;
    }

    // Handle Array
    if (obj instanceof Array) {
        copy = [];
        for (var i = 0, len = obj.length; i < len; i++) {
            copy[i] = clone(obj[i]);
        }
        return copy;
    }

    // Handle Object
    if (obj instanceof Object) {
        copy = {};
        for (var attr in obj) {
            if (obj.hasOwnProperty(attr)) copy[attr] = clone(obj[attr]);
        }
        return copy;
    }

    throw new Error("Unable to copy obj! Its type isn't supported.");
}

function strrchr (haystack, needle) {
  //  discuss at: https://locutus.io/php/strrchr/
  // original by: Brett Zamir (https://brett-zamir.me)
  //    input by: Jason Wong (https://carrot.org/)
  // bugfixed by: Brett Zamir (https://brett-zamir.me)
  //   example 1: strrchr("Line 1\nLine 2\nLine 3", 10).substr(1)
  //   returns 1: 'Line 3'
  var pos = 0;
  if (typeof needle !== 'string') {
    needle = String.fromCharCode(parseInt(needle, 10));
  }
  needle = needle.charAt(0);
  pos = haystack.lastIndexOf(needle);
  if (pos === -1) {
    return false;
  }
  return haystack.substr(pos);
}

function objname(obj) { return obj.toString().match(/ (\w+)/)[1]; };

function chkconf(sec,prop,type,def,flags) {
	if (typeof(sec) == "undefined") return;
	if (typeof(sec[prop]) != "undefined") return;
	config.add(objname(sec),prop,type,def,flags);
}

function pct(val1, val2) {
//	printf("val1: %f, val2: %f\n", val1,val2);
	var lv1 = val1;
	if (lv1 < 0) lv1 *= (-1);
	var lv2 = val2;
	if (lv2 < 0) lv2 *= (-1);
//	printf("lv1: %f, lv2: %f\n", lv1, lv2);
	if (lv1 > lv2)
		var d = lv2/lv1;
	else
		var d = lv1/lv2;
//	printf("d: %f\n", d);
	return 100 - (d * 100.0);
}

function rtest(obj) { return JSON.parse(JSON.stringify(obj)); }

function delobj(obj) {
	for(key in obj) delete obj[key];
	delete obj;
}

dumpobj = function(obj) {
        if (0 == 1) {
        printf("%s keys:\n",objname(obj));
        for(key in obj) printf("key: %s\n",key);
        printf("\n");
        return;
        }
        for(key in obj) {
                if (obj[key] && typeof obj[key] === 'function') continue;
                printf("%-30.30s: %s\n", key, obj[key]);
        }
}

if (!String.prototype.startsWith) {
	String.prototype.startsWith = function(searchString, position){
		position = position || 0;
		return ((this.substr(position, searchString.length) === searchString) ? true : false);
	};
}

time = function() { return systime() >>> 0; }
