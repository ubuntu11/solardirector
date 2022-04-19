
function write_main() {
	if (typeof(gc_done) == "undefined") {
		gc();
		gc_done = true;
	}
}
