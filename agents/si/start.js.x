
//var old = smanet.set("GdManStr","Start");
//printf("old: %s\n", old);
/* Takes the difference in seconds between two times and returns an 
   object that describes how many years, days, hours, minutes, and seconds have 
   elapsed.
   */
function CalculateTimeDifference(difference){
  
  var years = Math.floor(difference/(60*60*24*365));
  difference = difference % (60*60*24*365);
  
  var days = Math.floor(difference/(60*60*24));
  difference = difference % (60*60*24);
  
  var hours = Math.floor(difference/(60*60));
  difference = difference % (60*60);
  
  var minutes = Math.floor(difference/60);
  difference = difference % 60;
  
  var seconds = difference;
  
  return {years: years, days: days, hours: hours, minutes: minutes, seconds: seconds};
}

Console.write('Please enter your name: ');
var name = Console.read();
if (name === false){
  Console.writeErrorln('I/O Error: Unable to read from console.');
}
else {
  //Trim excess white space from the input.
  name = name.replace(/(^\s*|\s*$)/g, '');
  
  Console.write(name+', Please enter number of seconds to find time span for: ');
  var difference = Console.readInt();
  if (difference === false){
    Console.writeErrorln('I/O Error: Unable to read from console.');
  }
  else {
    Console.writeErrorln('Read: ', typeof difference, '(', difference, ')');
    //Gobble up the new line that is left on the input stream.
    Console.read();
    Console.write(name+', Please enter a random number between 0 and 1: ');
    var rand = Console.readFloat();
    if (rand === false){
      Console.writeErrorln('I/O Error: Unable to read from console.');
    }
    else {
      Console.writeErrorln('Read: ', typeof rand, '(', rand, ')');
      //Gobble up the new line that is left on the input stream.
      Console.read();
      Console.write('Alright ', name, ', are you ready?[yn]: ');
      var ready = Console.readChar();
      if (ready === false){
        Console.writeErrorln('I/O Error: Unable to read from console.');
      }
      else {
        Console.writeErrorln('Read: ', typeof ready, '(', ready, ')');
        if (ready == 'y' || ready == 'Y'){
          rand = Math.floor(rand*1000)/1000;
          Console.writeln('Adding a ', rand*100, ' percent margin.');
          difference += Math.floor(difference * rand);
          Console.writeln('New number of seconds is: '+difference);
          Console.write('Calculating...');
          var breakdown = CalculateTimeDifference(difference);
          Console.writeln('done');
          Console.writeln(breakdown.years, " years, ", breakdown.days, " days, ", 
                        breakdown.hours, " hours, ", breakdown.minutes, " minutes, and "+breakdown.seconds+" seconds\n");
        }
      }
    }  
  }
}
