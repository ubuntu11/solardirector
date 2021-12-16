
name=$(basename $(pwd))
if test ! -f ${name}.h; then
	echo "error: ${name}.h not found"
	exit 1
fi
PREFIX=$(echo $name | tr '[:lower:]' '[:upper:]')

have_start=0
open=0
tmp=/tmp/mkjs_${name}.dat
rm -f $tmp
while read line
do
	line="$(echo "$line" | sed 's:*::g')"
	test $(echo "$line" | grep -c "//") -gt 0 && continue
	echo "have_start: $have_start, open: $open"
	if test $(echo "$line" | grep -c "{") -gt 0;  then
		((open++))
	elif test $(echo "$line" | grep -c "}") -gt 0;  then
		((open--))
	fi
	if test $have_start -eq 1; then
		if test $open -eq 0; then
			break;
		else
			echo $line >> $tmp
		fi
	elif test $(echo "$line" | grep -c "struct si_session {") -gt 0; then
		have_start=1
	fi
done < ${name}.h

do_enums() {
out=${name}_propid.h
echo "#define ${PREFIX}_PROPIDS \\" > $out
while read n
do
#	echo $n
	t=$(echo $n | awk '{ print $1 }')
	v=$(echo $n | awk '{ print $2 }' | sed -e 's:\.:_:g' -e "s/[:;]//g" | awk -F'[' '{ print $1 }')
#	echo $t $v $p
	if test $(echo "$v" | grep -c '(') -gt 0; then
		continue
	elif test "$t" = "unsigned" -o "$t" = "uint16_t"; then
		:
	elif test "$t" = "char"; then
		:
	elif test "$t" = "int" -o "$t" = "time_t" -o "$t" = "uint32_t"; then
		:
	elif test "$t" = "float"; then
		:
	else
		continue
	fi
	p="${PREFIX}_PROPERTY_ID_$(echo $v | tr '[:lower:]' '[:upper:]'), \\"
	printf "\t%s\n" "$p" >> $out
done < $tmp
printf "\t%s\n" "${PREFIX}_PROPERTY_ID_MAX" >> $out
}
do_sw() {
out=${name}_getprop.h
echo "#define ${PREFIX}_GETPROP \\" > $out
while read n
do
#	echo $n
	t=$(echo $n | awk '{ print $1 }')
	v=$(echo $n | awk '{ print $2 }' | sed -e 's:\.:_:g' -e "s/[:;]//g" | awk -F'[' '{ print $1 }')
	p="${PREFIX}_PROPERTY_ID_$(echo $v | tr '[:lower:]' '[:upper:]'):"
#	echo $t $v $p
	if test $(echo "$v" | grep -c '(') -gt 0; then
		continue
	elif test "$t" = "unsigned" -o "$t" = "uint16_t"; then
		c="*rval = INT_TO_JSVAL(s->${v});"
	elif test "$t" = "char"; then
		c="*rval = STRING_TO_JSVAL(JS_NewStringCopyZ(cx,s->${v}));"
	elif test "$t" = "int" -o "$t" = "time_t" -o "$t" = "uint32_t"; then
		c="*rval = INT_TO_JSVAL(s->${v});"
	elif test "$t" = "float"; then
		c="JS_NewDoubleValue(cx, s->${v}, rval);"
	else
		continue
	fi
	printf "\t\tcase %s\n" "${p}\\" >> $out
	printf "\t\t\t%s\n" "${c}\\" >> $out
	printf "\t\t\t%s\n" "break;\\" >> $out
done < $tmp
printf "\t\tcase %s\n" "${PREFIX}_PROPERTY_ID_MAX:\\" >> $out
printf "\t\t\t%s\n" "*rval = INT_TO_JSVAL(${PREFIX}_PROPERTY_ID_MAX);\\" >> $out
printf "\t\t\t%s\n" "break;" >> $out
}
do_prop() {
out=${name}_propspec.h
echo "#define ${PREFIX}_PROPSPEC \\" > $out
while read n
do
#	echo $n
	t=$(echo $n | awk '{ print $1 }')
	v=$(echo $n | awk '{ print $2 }' | sed -e 's:\.:_:g' -e "s/[:;]//g" | awk -F'[' '{ print $1 }')
	if test $(echo "$v" | grep -c '(') -gt 0; then
		continue
	elif test "$t" = "unsigned" -o "$t" = "uint16_t"; then
		:
	elif test "$t" = "char"; then
		:
	elif test "$t" = "int" -o "$t" = "time_t" -o "$t" = "uint32_t"; then
		:
	elif test "$t" = "float"; then
		:
	else
		continue
	fi
	p="${PREFIX}_PROPERTY_ID_$(echo $v | tr '[:lower:]' '[:upper:]')"
	printf "\t\t{ \"${v}\",${p},JSPROP_ENUMERATE|JSPROP_READONLY },\\\\\n" >> $out
done < $tmp
printf "\t\t{ \"property_id_max\",${PREFIX}_PROPERTY_ID_MAX,JSPROP_ENUMERATE|JSPROP_READONLY },\n" >> $out
}
do_enums
do_sw
do_prop
rm -f $tmp
