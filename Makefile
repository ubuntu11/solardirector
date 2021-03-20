
all:
	for d in lib smanet; do make -j $$MAKE_JOBS -C $$d; done
	@for d in *; do if test -f $$d/Makefile; then make -j $$MAKE_JOBS -C $$d || exit 1; fi; done

install clean cleanall::
	for d in *; do if test -f $$d/Makefile; then make -C $$d $@; fi; done

list:
	@$(MAKE) -pRrq -f $(lastword $(MAKEFILE_LIST)) : 2>/dev/null | awk -v RS= -F: '/^# File/,/^# Finished Make data base/ {if ($$1 !~ "^[#.]") {print $$1}}' | sort | egrep -v -e '^[^[:alnum:]]' -e '^$@$$'
