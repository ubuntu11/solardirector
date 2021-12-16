
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

const arr1 = ['Steve','Tim','Bob'];
const arr2 = ['Julieanne','Jackie','John','Jim','Joe'];
var arr3 = arr1.concat(arr2).unique().sort(); 
var s = "[";
for(var i=0; i < arr3.length; i++) {
	if (i) s += ",";
	s += '\"' + arr3[i] + '\"';
//sprintf("\"%s\",",arr3[i]);
}
s += "]";
printf("%s\n",s);
var j = JSON.parse(s);
printf("%s\n",JSON.stringify(j,null,4));
