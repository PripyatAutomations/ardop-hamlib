subdirs += ARDOP1OFDM
subdirs += ARDOP2
#subdirs += ARDOP3
#subdirs += ARDOP3K
subdirs += ARDOPC
subdirs += ARDOPOFDM

install_files += ARDOP1OFDM/ardop1ofdm
install_files += ARDOP2/ardop2
#install_files += ARDOP3/ardop3
#install_files += ARDOP3k/ardop3k
install_files += ARDOPC/ardopc
install_files += ARDOPOFDM/ardopofdm

all:
	@for i in ${subdirs}; do \
	   echo "Making $@ in $$i"; \
	   ${MAKE} -s -C $$i $@; \
	done

clean:
	@for i in ${subdirs}; do \
	   echo "Making $@ in $$i"; \
	   ${MAKE} -s -C $$i clean; \
	done
	${RM} *.log

install: ${install_files}
	@for i in ${install_files}; do \
	   echo "Install $$i"; \
	   cp -f $$i /usr/bin/; \
	done

${install_files}: all
