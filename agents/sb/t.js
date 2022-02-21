var ch = "A".charCodeAt(0);
function nextChar(c) {
    return String.fromCharCode(c.charCodeAt(0) + 1);
}
for(i=0; i < 5; i++) {
    printf("%s\n",String.fromCharCode(ch+i));
}
