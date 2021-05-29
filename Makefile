
all:
	for d in lib smanet; do $(MAKE) -j $$MAKE_JOBS $(MAKEFLAGS) -C $$d; done
	@for d in *; do if test -f $$d/Makefile; then make -j $$MAKE_JOBS $(MAKEFLAGS) -C $$d || exit 1; fi; done

#	for d in *; do if test -f $$d/Makefile; then $(MAKE) $(MAKEFLAGS) -C $$d $@; fi; done
install release clean cleanall::
	for d in *; do if test -f $$d/Makefile; then $(MAKE) -C $$d $@; fi; done

list:
	@$(MAKE) -pRrq -f $(lastword $(MAKEFILE_LIST)) : 2>/dev/null | awk -v RS= -F: '/^# File/,/^# Finished Make data base/ {if ($$1 !~ "^[#.]") {print $$1}}' | sort | egrep -v -e '^[^[:alnum:]]' -e '^$@$$'


push: clean
	git add -A .
	git commit -m update
	git push
