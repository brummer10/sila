
	NAME = sila
	VER = 0.3
	PREFIX = /usr
	BIN_DIR = $(PREFIX)/bin
	SHARE_DIR = $(PREFIX)/share
	DESKAPPS_DIR = $(SHARE_DIR)/applications
	DEBNAME = $(NAME)_$(VER)
	CREATEDEB = dh_make -y -s -n -e $(USER)@org -p $(DEBNAME) -c gpl >/dev/null
	DIRS = $(BIN_DIR)  $(DESKAPPS_DIR)
	OVERWRITE1 = override_dh_strip:
	OVERWRITE2 = "	dh_strip --no-automatic-dbgsym"
	BUILDDEB = dpkg-buildpackage -rfakeroot -uc -us -b 2>/dev/null | grep dpkg-deb 

	CXXFLAGS = -Wall -ffast-math  `pkg-config --cflags gtkmm-3.0`
	LDFLAGS = `pkg-config --libs gtkmm-3.0 jack` -ldl

	OBJECTS = main.o sila_host.o sila_browser.o sila_ui.o

.PHONY : all clean install uninstall tar deb

all : $(NAME)

clean :
	rm -f $(NAME) $(OBJECTS)

install : all
	mkdir -p $(DESTDIR)$(BIN_DIR)
	mkdir -p $(DESTDIR)$(DESKAPPS_DIR)
	install $(NAME) $(DESTDIR)$(BIN_DIR)
	install $(NAME).desktop $(DESTDIR)$(DESKAPPS_DIR)

uninstall :
	@rm -rf $(BIN_DIR)/$(NAME) $(DESKAPPS_DIR)/$(NAME).desktop

tar : clean
	@cd ../ && \
	tar -cf $(NAME)-$(VER).tar.bz2 --bzip2 $(NAME)
	@echo "build $(NAME)-$(VER).tar.bz2"

deb : 
	@rm -rf ./debian
	@echo "create ./debian"
	-@ $(CREATEDEB)
	@ #@echo "touch ./debian/dirs"
	@echo $(DIRS) > ./debian/dirs
	@ #@echo "touch ./debian/rules"
	@echo $(OVERWRITE1) >> ./debian/rules
	@echo $(OVERWRITE2) >> ./debian/rules
	@echo "try to build debian package, that may take some time"
	-@ if $(BUILDDEB); then echo ". . , done"; else \
     echo "sorry, build fail"; fi
	@rm -rf ./debian

$(OBJECTS) : sila.h

$(NAME) : $(OBJECTS)
	$(CXX) $(OBJECTS) $(LDFLAGS) -o $(NAME)
