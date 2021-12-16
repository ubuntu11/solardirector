
const obj = {
    name: 'Atta',
    profession: 'Software Engineer',
    country: 'PK',
    skills: ['Java', 'Spring Boot', 'Node.js', 'JavaScript']
};
// serialize JSON object
const str = JSON.stringify(obj, ['name', 'country', 'skills'] , '\t');
console.log(str);

var text = "{ \"name\": \"pack_10\", \"capacity\": 75, \"voltage\": 44.93000030517578, \"current\": 0, \"ntemps\": 2, \"temps\": [ 23.100000381469727, 23 ], \"ncells\": 14, \"cellvolt\": [ 3.1730000972747803, 3.2179999351501465, 3.1710000038146973, 3.1570000648498535, 3.2219998836517334, 3.196000099182129, 3.174999952316284, 3.2130000591278076, 3.1449999809265137, 3.1659998893737793, 3.2260000705718994, 3.2709999084472656, 3.305000066757202, 3.296999931335449 ], \"cellres\": [ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 ], \"cell_min\": 3.1449999809265137, \"cell_max\": 3.305000066757202, \"cell_diff\": 0.16000008583068848, \"cell_avg\": 3.2096428871154785, \"cell_total\": 44.935001373291016, \"errcode\": 0, \"errmsg\": \"\", \"state\": \"Charging,Discharging,Balancing\" }";
var j = JSON.parse(text);
j.another_field = "adding";
printf("j: %s\n",j);
var s = JSON.stringify(j,null,4);
console.log("s: %s",s);
