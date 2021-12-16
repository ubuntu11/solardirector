//Lets test some sockets.
try {
    var socket = new Socket();
    Console.writeln("Socket object created successfully.");

    //Check the constants
    Console.writeln("SOL_TCP = " + Socket.SOL_TCP);

    //Connect to my website
    //socket.connect({});
    socket.connect({family:Socket.PF_INET, details:{port:80,address:"aoeex.com"}});

    //Make a request
    socket.write("GET / HTTP/1.0\r\nHost: aoeex.com\r\n\r\n");

    //Read the response
    while ((data=socket.read()).length > 0){
        //Read the banner
        Console.writeln(data);
    }

    //Close up shop
    socket.close();
}
catch (e){
    Console.writeln ("\n~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~");
    Console.writeln("Caught exception. ");
    Console.writeln(e);
}
