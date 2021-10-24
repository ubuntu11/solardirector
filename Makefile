
all:
	for d in core smanet js transports roles; do $(MAKE) -C $$d; done
	@for d in *; do if test -f $$d/Makefile; then make -C $$d || exit 1; fi; done

install release clean cleanall::
	for d in *; do if test -f $$d/Makefile; then $(MAKE) -C $$d $@; fi; done

list:
	@$(MAKE) -pRrq -f $(lastword $(MAKEFILE_LIST)) : 2>/dev/null | awk -v RS= -F: '/^# File/,/^# Finished Make data base/ {if ($$1 !~ "^[#.]") {print $$1}}' | sort | egrep -v -e '^[^[:alnum:]]' -e '^$@$$'


push: clean
	git add -A .
	git commit -m update
	git push
