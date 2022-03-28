
/*
Copyright (c) 2022, Stephen P. Shoecraft
All rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the root directory of this source tree.
*/

include(dirname(script_name)+"/../../core/init.js");

function getshort(data,index) {
	var s = (data[index] & 0xF0) << 8 | data[index+1] & 0x0F;
	return s;
}

function putshort(data,index,num) {
	var hex = sprintf("%04x",num);
	data[index++] = "0x" + hex.slice(2,4);
	data[index++] = "0x" + hex.slice(0,2);
};

function putlong(data,index,num) {
	var hex = sprintf("%08x",num);
//	dprintf(1,"hex: %s\n", hex);
	data[index++] = "0x" + hex.slice(6,8);
	data[index++] = "0x" + hex.slice(4,6);
	data[index++] = "0x" + hex.slice(2,4);
	data[index++] = "0x" + hex.slice(0,2);
};

function si_isvrange(v) { return ((v >= SI_VOLTAGE_MIN) && (v  <= SI_VOLTAGE_MAX)) }
function HOURS(n) { return(n * 3600); }
function MINUTES(n) { return(n * 60); }
function float_equals(n1,n2) { return(n1 === n2); }


/* global vars */
info = si.info;
script_dir = dirname(script_name);

function init_main() {
	// Call init funcs
	var init_funcs = [
		"charge",
		"sim",
//		"read",
		"write"
	];
	for(i=0; i < init_funcs.length; i++) {
		if (run(script_dir+"/"+init_funcs[i]+".js",init_funcs[i]+"_init"))
			exit(1);
	}
}
