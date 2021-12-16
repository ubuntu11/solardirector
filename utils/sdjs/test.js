//Create a log file for use later.
if (!File.exists("runlog")){
    File.create("runlog");
}
else {
    File.touch("runlog");
}

String.prototype.trim = function(){ return this.replace(/^\s*/, '').replace(/\s*$/, ''); }

Console.writeln("Enter a filename: ");
var strfile = Console.read().trim();

Console.writeln("Creating file object for "+strfile);
var file = new File(strfile);

var done = false;
do {
    
    Console.writeln("Do what with "+strfile+"?");
    Console.writeln("1) Check if exists. ");
    Console.writeln("2) Update modifcation time. ");
    Console.writeln("3) Create file. ");
    Console.writeln("4) Quit. ");
    var choice = parseInt(Console.read());
    if (choice == 1){
        if (file.exists()){
            Console.writeln("Yeppers, file exists.");
        }
		else {
			Console.writeln("Nope, it doesn't exist yet.");
		}
    }
    else if (choice == 2){
        try {
            if (file.touch()){
                Console.writeln("File access and modification time update.");
            }
        }
        catch (e){
            Console.writeln("Time update failed, Exception caught: ");
            Console.writeln(e);
        }
    }
    else if (choice == 3){
		try
		{
	        file.create();
		}
		catch (e)
		{
			Console.writeln("Creation failed. Exception caught: ");
			Console.writeln(e);
		}
    }
    else if (choice == 4){
        done = true;
    }
} while (done == false);

