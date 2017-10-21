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


#include "sila.h"

/*****************************************************************************/

namespace sila {

int LadspaHost::compute_callback(jack_nframes_t nframes, void *arg) {
    LadspaHost& self = *static_cast<LadspaHost*>(arg);

    for (int i = 0; i < static_cast<int>(self.ladspa_plug.descriptor->PortCount); i++) {
        if (LADSPA_IS_PORT_AUDIO(self.ladspa_plug.descriptor->PortDescriptors[i])) {
            self.ladspa_plug.descriptor->connect_port (self.ladspa_plug.lhandle, i,
                (float *)jack_port_get_buffer(self.ports[i], nframes));
        }
    }
    self.ladspa_plug.descriptor->run(self.ladspa_plug.lhandle, nframes);

    return 0;
}

/*****************************************************************************/

gboolean LadspaHost::buffer_changed(void* arg) {
    fprintf(stderr,"buffer size changed\n");
    return false;
}

/*****************************************************************************/

int LadspaHost::buffer_size_callback(jack_nframes_t nframes, void *arg) {
    LadspaHost& self = *static_cast<LadspaHost*>(arg);
    int _bz = jack_get_buffer_size(self.jack_client);
    if (_bz != self.BZ) {
        g_idle_add(buffer_changed,(void*)arg);
    }
    return 0;
}

/*****************************************************************************/

void LadspaHost::shut_down_callback(void *arg) {
    LadspaHost* self = static_cast<LadspaHost*>(arg);
    fprintf(stderr,"Exit: JACK shut us down\n");
    delete [] self->ladspa_plug.cpv;
    delete [] self->ports;
    delete self;
    if(Counter::get_instance()->getCount() <=0) {
        if(Gtk::Main::instance()->level()) Gtk::Main::quit();
        exit(1);
    }
}

/*****************************************************************************/

void LadspaHost::deactivate_jack() {
    get_connections();
    jack_deactivate(jack_client);
}

/*****************************************************************************/

void LadspaHost::activate_jack() {
    if (jack_activate(jack_client)) {
        delete [] ladspa_plug.cpv;
        delete [] ports;
        fprintf(stderr,"Exit: could not activate JACK processing\n");
        delete this;
        if(Counter::get_instance()->getCount() <=0) { 
            if(Gtk::Main::instance()->level()) Gtk::Main::quit();
            exit(1);
        }
    } else {
        std::string portname;
        int a = 1;
        int b = 1;
        int c = 0;
        int d = 0;
        for (int i = 0; i < static_cast<int>(ladspa_plug.descriptor->PortCount); i++) {
            if (LADSPA_IS_PORT_CONTROL(ladspa_plug.descriptor->
                                   PortDescriptors[i]) &&
                LADSPA_IS_PORT_INPUT(ladspa_plug.descriptor->PortDescriptors[i])) {
                continue;
            }
            if (LADSPA_IS_PORT_INPUT(ladspa_plug.descriptor->PortDescriptors[i])) {
                portname = ladspa_plug.descriptor->Name;
                portname += "_SHINPUT";
                c++;
                portname += to_string(a);
                portname += "_";
                portname += to_string(c);
                while(getenv(portname.c_str()) != NULL){
                    jack_connect(jack_client, getenv(portname.c_str()), jack_port_name(ports[i]));
                    portname = ladspa_plug.descriptor->Name;
                    portname += "_SHINPUT";
                    c++;
                    portname += to_string(a);
                    portname += "_";
                    portname += to_string(c);
                }
                c--;
                a++;
            } else if (LADSPA_IS_PORT_OUTPUT(ladspa_plug.descriptor->PortDescriptors[i])){
                portname = ladspa_plug.descriptor->Name;
                portname += "_SHOUTPUT";
                d++;
                portname += to_string(b);
                portname += "_";
                portname += to_string(d);
                while (getenv(portname.c_str()) != NULL) {
                    jack_connect(jack_client, jack_port_name(ports[i]), getenv(portname.c_str()));
                    portname = ladspa_plug.descriptor->Name;
                    portname += "_SHOUTPUT";
                    d++;
                    portname += to_string(b);
                    portname += "_";
                    portname += to_string(d);
                }
                d--;
                b++;
            }
        }
    }
}

/*****************************************************************************/

bool LadspaHost::init_jack(Glib::ustring *message) {
    client_name = "ladspa_";
    jack_status_t jack_status;
    int i;
    unsigned long flags;
    ladspa_plug.n_audio_in = 0;
    ladspa_plug.n_audio_out = 0;
    ladspa_plug.cnp = 0;
    ladspa_plug.cpv = NULL;
    ports = NULL;

    client_name += ladspa_plug.descriptor->Label;
    jack_client = jack_client_open(client_name.c_str(),
                                          JackNullOption, &jack_status);
    if (!jack_client) {
        *message = "Error: could not connect to JACK";
        return false;
    }

    try {
        ports = new jack_port_t* [ladspa_plug.descriptor->PortCount]();
        ladspa_plug.cpv = new float [ladspa_plug.descriptor->PortCount * sizeof(float)];
    } catch(...) {
        *message = "Error: could not allocate memory";
        return false;
    }

    for (i = 0; i < static_cast<int>(ladspa_plug.descriptor->PortCount); i++) {
        if (LADSPA_IS_PORT_CONTROL(ladspa_plug.descriptor->
                                   PortDescriptors[i]) &&
            LADSPA_IS_PORT_INPUT(ladspa_plug.descriptor->PortDescriptors[i])) {
            ladspa_plug.cnp++;
            continue;
        }

        if (LADSPA_IS_PORT_INPUT(ladspa_plug.descriptor->PortDescriptors[i]) &&
            LADSPA_IS_PORT_AUDIO(ladspa_plug.descriptor->PortDescriptors[i])) {
            flags = JackPortIsInput;
            ladspa_plug.n_audio_in++;
            
        } else if (LADSPA_IS_PORT_OUTPUT(ladspa_plug.descriptor->PortDescriptors[i]) &&
            LADSPA_IS_PORT_AUDIO(ladspa_plug.descriptor->PortDescriptors[i])){
            flags = JackPortIsOutput;
            ladspa_plug.n_audio_out++;
        }

        ports[i] =
            jack_port_register(jack_client,
                               ladspa_plug.descriptor->PortNames[i],
                               JACK_DEFAULT_AUDIO_TYPE,
                               flags, 0);

        if (!ports[i]) {
            delete [] ladspa_plug.cpv;
            delete [] ports;
            *message = "Error: could not register JACK ports";
            return false;
        }
    }

    if (!ladspa_plug.n_audio_in) {
        fprintf(stderr,"Warning: plugin have no input JACK port \n");
    } 
    if (!ladspa_plug.n_audio_out) {
        fprintf(stderr,"Warning: plugin have no output JACK port \n");
    }

    jack_set_process_callback(jack_client, compute_callback, this);
    jack_set_buffer_size_callback(jack_client, buffer_size_callback, this);
    jack_on_shutdown(jack_client, shut_down_callback, this);

    SR = jack_get_sample_rate(jack_client);
    BZ = jack_get_buffer_size(jack_client);

    ladspa_plug.lhandle = ladspa_plug.descriptor->instantiate(ladspa_plug.descriptor, SR);
    if (!ladspa_plug.lhandle) {
        *message = "Error: could not instantiate the plugin.";
        jack_cleanup();
        return false;
    }

    for (i = 0; i < static_cast<int>(ladspa_plug.descriptor->PortCount); i++) {
        if (LADSPA_IS_PORT_CONTROL(ladspa_plug.descriptor-> PortDescriptors[i]) &&
            LADSPA_IS_PORT_INPUT(ladspa_plug.descriptor->PortDescriptors[i])) {
            ladspa_plug.cpv[i] = 0.0;
            ladspa_plug.descriptor->connect_port(ladspa_plug.lhandle, i,&ladspa_plug.cpv[i]);
        }
        if (LADSPA_IS_PORT_CONTROL(ladspa_plug.descriptor-> PortDescriptors[i]) &&
            LADSPA_IS_PORT_OUTPUT(ladspa_plug.descriptor->PortDescriptors[i])) {
            ladspa_plug.cpv[i] = 0.0;
            ladspa_plug.descriptor->connect_port(ladspa_plug.lhandle, i,&ladspa_plug.cpv[i] );
        }
    }

    if (ladspa_plug.descriptor->activate) {
        ladspa_plug.descriptor->activate(ladspa_plug.lhandle);
    }

    return true;
}

/*****************************************************************************/

void LadspaHost::get_connections() {
    const char** port = NULL;
    std::string portname;
    int a = 0;
    int b = 0;
    int c = 0;
    int d = 0;
    for (int i = 0; i < static_cast<int>(ladspa_plug.descriptor->PortCount); i++) {
        if (LADSPA_IS_PORT_CONTROL(ladspa_plug.descriptor-> PortDescriptors[i]) &&
            LADSPA_IS_PORT_INPUT(ladspa_plug.descriptor->PortDescriptors[i])) {
            continue;
        }
        if (LADSPA_IS_PORT_INPUT(ladspa_plug.descriptor->PortDescriptors[i])) {
            a++;
            if (jack_port_connected(ports[i])) {
                port = jack_port_get_connections(ports[i]);
                for ( int j = 0; port[j] != NULL; j++) {
                    portname = ladspa_plug.descriptor->Name;
                    portname += "_SHINPUT";
                    c++;
                    portname += to_string(a);
                    portname += "_";
                    portname += to_string(c);
                    setenv(portname.c_str(),port[j],1);
                }
            }
        } else if (LADSPA_IS_PORT_OUTPUT(ladspa_plug.descriptor->PortDescriptors[i])){
            b++;
            if (jack_port_connected(ports[i])) {
                port = jack_port_get_connections(ports[i]);
                for ( int j = 0; port[j] != NULL; j++) {
                    portname = ladspa_plug.descriptor->Name;
                    portname += "_SHOUTPUT";
                    d++;
                    portname += to_string(b);
                    portname += "_";
                    portname += to_string(d);
                    setenv(portname.c_str(),port[j],1);
                }
            }
        }
    }
    free(port);
}

/*****************************************************************************/

void LadspaHost::jack_cleanup() {
    if (jack_client) {
        get_connections();

        jack_deactivate(jack_client);

        if (ladspa_plug.descriptor->deactivate) {
            ladspa_plug.descriptor->deactivate(ladspa_plug.lhandle);
        }
        if (ladspa_plug.descriptor->cleanup) {
            ladspa_plug.descriptor->cleanup(ladspa_plug.lhandle);
        }

        for (int i = 0; i < static_cast<int>(ladspa_plug.descriptor->PortCount); i++) {
            if (LADSPA_IS_PORT_CONTROL(ladspa_plug.descriptor-> PortDescriptors[i]) &&
                LADSPA_IS_PORT_INPUT(ladspa_plug.descriptor->PortDescriptors[i])) {
                continue;
            }
            if (LADSPA_IS_PORT_INPUT(ladspa_plug.descriptor->PortDescriptors[i])) {
                jack_port_unregister(jack_client, ports[i]);
            } else if (LADSPA_IS_PORT_OUTPUT(ladspa_plug.descriptor->PortDescriptors[i])){
                jack_port_unregister(jack_client, ports[i]);
            }
        }
        
        if (ladspa_plug.PluginHandle) dlclose(ladspa_plug.PluginHandle);
        jack_client_close(jack_client);
        jack_client = NULL;
        delete [] ladspa_plug.cpv;
        delete [] ports;
    }
}

/*****************************************************************************/

bool LadspaHost::load_ladspa_plugin(Glib::ustring PluginFilename, 
               int id, Glib::ustring *message) {
    if(PluginFilename.empty()) return false;
    LADSPA_Descriptor_Function phDescriptorFunction;
    const LADSPA_Descriptor * phDescriptor;
    char * pch;
    std::string path;

    pch=strchr(const_cast<char*>(PluginFilename.c_str()),'/');
    if(pch == NULL) {
        char * p = getenv ("LADSPA_PATH");
        if (p != NULL) {
            path = getenv ("LADSPA_PATH");
        } else {
            setenv("LADSPA_PATH","/usr/lib/ladspa",0);
            setenv("LADSPA_PATH","/usr/local/lib/ladspa",0);
            path = getenv("LADSPA_PATH");
        } 
        path += "/";
        path += PluginFilename;
        PluginFilename = const_cast<char*>(path.c_str());
    }
    
    ladspa_plug.PluginHandle = dlopen(PluginFilename.c_str(), RTLD_NOW | RTLD_LOCAL);
    if (!ladspa_plug.PluginHandle) {
        *message = dlerror();
        jack_cleanup();
        return false;
    }

    phDescriptorFunction 
        = (LADSPA_Descriptor_Function)dlsym(ladspa_plug.PluginHandle, "ladspa_descriptor");
    if (!phDescriptorFunction) {
        *message = dlerror();
        jack_cleanup();
        return false;
    }

    for (int i = 0;; i++) {
        phDescriptor = phDescriptorFunction(i);
        if (!phDescriptor) {
            ladspa_plug.descriptor = phDescriptorFunction(0);
            break;
            if (!ladspa_plug.descriptor) {
                *message = "Error: ID not found in the given Plugin";
                jack_cleanup();
                return false;
            }
        } else if (static_cast<int>(phDescriptor->UniqueID) == id) {
            ladspa_plug.descriptor = phDescriptor;
            break;
        }
    }
    return true;
}


/*****************************************************************************/

bool LadspaHost::cmd_parser(int argc, char **argv,
            Glib::ustring *plfile, int *plid, Glib::ustring *message) {

    if (argc >= 3) {
        *plfile = argv[1];
        *plid = atoi(argv[2]);
    } else if  (argc >= 2) {
        *plfile = argv[1];
        *plid = 0;
    } else {
        *message = "Error: no plugin filename and no plugin ID given";
        return false;
    }
    return true;
}

/*****************************************************************************/

bool LadspaHost::on_delete_event(GdkEventAny*) {
    jack_cleanup();
    delete this;
    if(Counter::get_instance()->getCount() <=0) {
        if(Gtk::Main::instance()->level()) Gtk::Main::quit();
    }
    return false;
}

/*****************************************************************************/

bool LadspaHost::sila_try(int argc, char **argv) {
    Glib::ustring message;
    Glib::ustring plfile;
    int plid;

    jack_client = 0;

    if(!cmd_parser(argc, argv, &plfile, &plid, &message) ||
        !load_ladspa_plugin(plfile, plid, &message) ||
        !init_jack(&message)) {
        fprintf(stderr, "%s.\n", message.c_str());
        return false;
    }
    return true;
}

/*****************************************************************************/

void LadspaHost::sila_start(int argc, char **argv) {
    if (sila_try(argc, argv)) {
        m_window->signal_delete_event().connect(
            sigc::mem_fun(*this,&LadspaHost::on_delete_event)); 
        m_window->add(*create_widgets());
        m_window->resize(500, m_window->get_height());
        m_window->set_title(client_name.c_str());
        m_window->set_position(Gtk::WIN_POS_CENTER);
        m_window->show_all_children();
        activate_jack();
        m_window->show();
    } else {
        delete this;
        if(Counter::get_instance()->getCount() <=0) {
            if(Gtk::Main::instance()->level()) Gtk::Main::quit();
            exit(1);
        }
    }
}

} //end namespace sila
/*****************************************************************************/
