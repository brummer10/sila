/*
 * This is free and unencumbered software released into the public domain.

 * Anyone is free to copy, modify, publish, use, compile, sell, or
 * distribute this software, either in source code form or as a compiled
 * binary, for any purpose, commercial or non-commercial, and by any
 * means.

 * In jurisdictions that recognize copyright laws, the author or authors
 * of this software dedicate any and all copyright interest in the
 * software to the public domain. We make this dedication for the benefit
 * of the public at large and to the detriment of our heirs and
 * successors. We intend this dedication to be an overt act of
 * relinquishment in perpetuity of all present and future rights to this
 * software under copyright law.

 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#pragma once

#ifndef _SILA_H
#define _SILA_H

#include <jack/jack.h>
#include <ladspa.h>

#include <gtkmm.h>

#include <dlfcn.h>
#include <stdio.h>
#include <string.h>
#include <iostream>
#include <sstream>
#include <fstream>
#include <string>
#include <errno.h>
#include <stdlib.h>
#include <list>

/*****************************************************************************/

namespace sila {

class Counter {
private:
    Counter() {}
    ~Counter() {};
public:
    static int & getCount() {
        static int Count = 0;
        return Count;
    }
    static Counter *get_instance() {
        static Counter instance;
        return &instance;
    };
};

class LadspaBrowser : public Gtk::Window {
private:

    class LadspaHandle {
     public:
        Glib::ustring libname;
        Glib::ustring labelname;
        Glib::ustring UnicID;
    };

    class ModelColumns : public Gtk::TreeModel::ColumnRecord {
     public:
        ModelColumns() { add(name);
                         add(label);}
        Gtk::TreeModelColumn<Glib::ustring> name;
        Gtk::TreeModelColumn<Glib::ustring> label;
    };
    std::list<Glib::ustring> plug_mono_list;
    std::list<Glib::ustring> plug_stereo_list;
    std::list<Glib::ustring> plug_misc_list;
    char *argv[3];
    ModelColumns columns;
    LadspaHandle plhandle;
    //Gtk::Dialog window;
    Gtk::VBox topBox;
    Gtk::HBox ButtonBox;
    Gtk::Button b1;
    Gtk::Button apply_button;
    Gtk::Button quit_button;
    Gtk::Image stck1;
    Gtk::Image stck2;
    Gtk::ScrolledWindow scrollWindow;
    Gtk::ScrolledWindow detailWindow;
    Gtk::TextView m_TextView;
    Gtk::VPaned m_pane;
    Glib::RefPtr<Gtk::TextBuffer> m_refTextBuffer;
    Gtk::TreeView treeview;
    Glib::RefPtr<Gtk::TreeStore> model;
    void on_pressed1_event();
    void on_execute();
    void on_quit();
    void on_cursor_changed();
    void create_list();
    void fill_buffers(Glib::ustring);
    void make_menu(const LADSPA_Descriptor * psDescriptor, Glib::ustring str);
    Glib::ustring analysePlugin(const char * pcPluginFilename, 
        const char * pcPluginLabel, const int bVerbose);
    void unloadLADSPAPluginLibrary(void * pvLADSPAPluginLibrary);
    void * loadLADSPAPluginLibrary(const char * pcPluginFilename);
    void * dlopenLADSPA(const char * pcFilename, int iFlag);
    void on_treeview_row_activated(const Gtk::TreeModel::Path& path, 
        Gtk::TreeViewColumn*);

public:
    void show_browser();
    void get_file_handle(Glib::ustring *plfile, int *plid);
    explicit LadspaBrowser();
    ~LadspaBrowser();
}; 

/*****************************************************************************/

template <class T>
inline std::string to_string(const T& t) {
    std::stringstream ss;
    ss << t;
    return ss.str();
}

class LadspaHost {
 private:

    class LadspaPlug {
     public:
        int n_audio_in;
        int n_audio_out;
        int cnp;
        float *cpv;
        const LADSPA_Descriptor *descriptor;
        LADSPA_Handle lhandle;
        void *PluginHandle;
    };

    bool cmd_parser( int argc, char **argv, Glib::ustring *plfile,
               int *plid, Glib::ustring *error);
    bool init_jack(Glib::ustring *error);
    bool load_ladspa_plugin(Glib::ustring PluginFilename, int id, Glib::ustring *error);
    bool sila_try(int argc, char **argv);
    bool on_delete_event(GdkEventAny*);
    int SR;
    int BZ;
    jack_client_t *jack_client;
    Gtk::Window *create_widgets();
    std::string client_name;
    jack_port_t **ports;
    void get_connections();
    static int compute_callback(jack_nframes_t nframes, void *arg);
    static int buffer_size_callback(jack_nframes_t nframes, void *arg);
    static gboolean buffer_changed(void* arg);
    static void shut_down_callback(void *arg);

 public:
    LadspaPlug ladspa_plug;
    Gtk::Window *m_window;
    int get_samplerate() {return SR;}
    void sila_start(int argc, char **argv);
    void jack_cleanup();
    void activate_jack();
    void deactivate_jack();
    LadspaHost() { 
        ++Counter::get_instance()->getCount();
        m_window = new Gtk::Window;
    }
    ~LadspaHost() { 
        --Counter::get_instance()->getCount();
        delete m_window;
    }
};

} //end namespace sila
/*****************************************************************************/

#endif // _SILA_H
