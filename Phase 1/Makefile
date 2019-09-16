
SUBDIRS= phase1a phase1b phase1c phase1d

.PHONY: $(SUBDIRS) all clean install subdirs

all: $(SUBDIRS)

subdirs: $(SUBDIRS)

clean: $(SUBDIRS)
	rm -f p3/*.o

install: $(SUBDIRS)

$(SUBDIRS):
	$(MAKE) -C $@ $(MAKECMDGOALS)