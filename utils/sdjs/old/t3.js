var names = ["David", "Cynthia", "Raymond", "Clayton", "Jennifer"];
print("Enter a name to search for: ");
var name = readline();
var position = names.indexOf(name);
print("position: "+position);
if (position >= 0) {
    print("Found " + name + " at position " + position);
}
else {
    print(name + " not found in array.");
}
