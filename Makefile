subdirs += ARDOP1OFDM
subdirs += ARDOP2
subdirs += ARDOP3
subdirs += ARDOP3K
subdirs += ARDOPC
subdirs += ARDOPOFDM

all clean install:
	@for i in ${subdirs}; do \
	   echo "Making $@ in $$i"; \
	   ${MAKE} -C $$i $@; \
	done
