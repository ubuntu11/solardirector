load("perfect.js");
print(perfect);
dis(perfect);
print();
for (var ln = 0; ln <= 40; ln++) {
    var pc = line2pc(perfect, ln);
    var ln2 = pc2line(perfect, pc);
    print("\tline " + ln + " => pc " + pc + " => line " + ln2);
}
