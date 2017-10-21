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

namespace sila_browser {

/*  this routine will search the LADSPA_PATH for the file. */
void *LadspaBrowser::dlopenLADSPA(const char * pcFilename, int iFlag) {

    char * pcBuffer = NULL;
    const char * pcEnd;
    const char * pcLADSPAPath;
    const char * pcStart;
    int iNeedSlash;
    size_t iFilenameLength;
    void * pvResult;
    iFilenameLength = strlen(pcFilename);
    pvResult = NULL;

    pcLADSPAPath = getenv("LADSPA_PATH");

    if (pcLADSPAPath == NULL) {
        setenv("LADSPA_PATH","/usr/lib/ladspa",0);
        setenv("LADSPA_PATH","/usr/local/lib/ladspa",0);
        pcLADSPAPath = getenv("LADSPA_PATH");
    } 
    pcStart = pcLADSPAPath;
    while (*pcStart != '\0') {
        pcEnd = pcStart;
        while (*pcEnd != ':' && *pcEnd != '\0')
            pcEnd++;
        pcBuffer = (char*)malloc(iFilenameLength + 2 + (pcEnd - pcStart));
        if (pcEnd > pcStart)
            strncpy(pcBuffer, pcStart, pcEnd - pcStart);
        iNeedSlash = 0;
        if (pcEnd > pcStart)
            if (*(pcEnd - 1) != '/') {
                iNeedSlash = 1;
                pcBuffer[pcEnd - pcStart] = '/';
            }
        strcpy(pcBuffer + iNeedSlash + (pcEnd - pcStart), pcFilename);
        pvResult = dlopen(pcBuffer, iFlag);
        free(pcBuffer);
        return pvResult;
    
    }
    return pvResult;
}

/*****************************************************************************/

void *LadspaBrowser::loadLADSPAPluginLibrary(const char * pcPluginFilename) {

    void * pvPluginHandle;

    pvPluginHandle = dlopenLADSPA(pcPluginFilename, RTLD_LOCAL|RTLD_NOW);
    if (!pvPluginHandle) {
        fprintf(stderr, 
            "Failed to load plugin \"%s\": %s\n", 
            pcPluginFilename,
            dlerror());
        //exit(1);
    }
    return pvPluginHandle;
}

/*****************************************************************************/

void LadspaBrowser::unloadLADSPAPluginLibrary(void * pvLADSPAPluginLibrary) {
    dlclose(pvLADSPAPluginLibrary);
}

/*****************************************************************************/

Glib::ustring LadspaBrowser::analysePlugin(const char * pcPluginFilename,
        const char * pcPluginLabel, const int bVerbose) {

    LADSPA_Descriptor_Function pfDescriptorFunction;
    const LADSPA_Descriptor * psDescriptor;
    unsigned long lPluginIndex;
    unsigned long lPortIndex;
    void * pvPluginHandle;
    LADSPA_PortRangeHintDescriptor iHintDescriptor;
    LADSPA_Data fBound;
    LADSPA_Data fDefault;
    Glib::ustring details;
    std::string collect_details;
    std::string collect_first;
    int inputports = 0;
    int outputports = 0;

    pvPluginHandle = loadLADSPAPluginLibrary(pcPluginFilename);

    dlerror();
    pfDescriptorFunction 
        = (LADSPA_Descriptor_Function)dlsym(pvPluginHandle, "ladspa_descriptor");
    if (!pfDescriptorFunction) {
    const char * pcError = dlerror();
    if (pcError) {
        fprintf(stderr,
            "Unable to find ladspa_descriptor() function in plugin file "
            "\"%s\": %s.\n"
            "Are you sure this is a LADSPA plugin file?\n", 
            pcPluginFilename,
            pcError);
        }
        return pcError;
    }
  
    for (lPluginIndex = 0;; lPluginIndex++) {
        inputports = 0;
        outputports = 0;
        psDescriptor = pfDescriptorFunction(lPluginIndex);
        if (!psDescriptor)
            break;
        if (pcPluginLabel != NULL)
            if (strcmp(pcPluginLabel, psDescriptor->Label) != 0)
        continue;

        if ( bVerbose == -1) {
            collect_details += psDescriptor->Label;
            collect_details += "\n";
        } else if (!bVerbose) {
            
            if (psDescriptor->PortCount == 0)
                collect_details +="\tERROR: PLUGIN HAS NO PORTS.\n";

            collect_details +="\n";

            for (lPortIndex = 0; 
                lPortIndex < psDescriptor->PortCount; 
                lPortIndex++) {
                if  (LADSPA_IS_PORT_AUDIO(psDescriptor->PortDescriptors[lPortIndex]) && 
                    LADSPA_IS_PORT_INPUT(psDescriptor->PortDescriptors[lPortIndex])) {
                        inputports ++;
                } else if  (LADSPA_IS_PORT_AUDIO(psDescriptor->PortDescriptors[lPortIndex]) && 
                    LADSPA_IS_PORT_OUTPUT(psDescriptor->PortDescriptors[lPortIndex])) {
                        outputports ++;
                }
                
            }
            if(inputports == 1 && outputports == 1) {
                collect_details += "\n Type    :  MONO";
            } else if(inputports == 2 && outputports == 2) {
                collect_details += "\n Type    :  STEREO";
            } else {
                collect_details += "\n Type    :  MISC";
            }
            
            collect_details +="\n Plugin Label: ";
            collect_details += psDescriptor->Label;
            collect_details +="\n Plugin Unique ";
            collect_details +=sila::to_string(psDescriptor->UniqueID);
            collect_details +="\n Plugin Name: ";
            collect_details +=psDescriptor->Name;
            
            
            
        } else {
            collect_details +="\n Plugin Name: ";
            collect_details +=psDescriptor->Name;
            collect_details +="\n Plugin Label: ";
            collect_details += psDescriptor->Label;
            collect_details += "\n Plugin Unique ";
            collect_details +=sila::to_string(psDescriptor->UniqueID);
            collect_details +="\n Maker: ";
            collect_details +=psDescriptor->Maker;
            collect_details +="\n";
            collect_details +=" RT: ";
            collect_details +=sila::to_string(psDescriptor->Properties);
            collect_details +="\n";
            collect_details +=" Ports: ";
            
            
            if (psDescriptor->PortCount == 0)
                collect_details +="\tERROR: PLUGIN HAS NO PORTS.\n";
            else collect_details += sila::to_string(psDescriptor->PortCount);
                collect_details +="\n";
      
            for (lPortIndex = 0; 
                lPortIndex < psDescriptor->PortCount; 
                lPortIndex++) {
                    collect_details +=" index ";
                    collect_details +=sila::to_string(lPortIndex);
                    collect_details += ": ";
                    collect_details += " ";
                    collect_details += psDescriptor->PortNames[lPortIndex];
                
            
            if (LADSPA_IS_PORT_CONTROL
                (psDescriptor->PortDescriptors[lPortIndex]) 
                && LADSPA_IS_PORT_AUDIO
                (psDescriptor->PortDescriptors[lPortIndex])){
                collect_details +=", ERROR: CONTROL AND AUDIO";
            } else if  (LADSPA_IS_PORT_CONTROL
                 (psDescriptor->PortDescriptors[lPortIndex])){
                collect_details +=" -> Type control";
                
            } else if  (LADSPA_IS_PORT_AUDIO
                 (psDescriptor->PortDescriptors[lPortIndex])){
                collect_details +=" -> Type audio";
                
            } else  {
                collect_details +=", ERROR: NEITHER CONTROL NOR AUDIO";
            }
            
            if (LADSPA_IS_PORT_INPUT
                (psDescriptor->PortDescriptors[lPortIndex])
                && LADSPA_IS_PORT_OUTPUT
                (psDescriptor->PortDescriptors[lPortIndex]))
                collect_details +="ERROR: INPUT AND OUTPUT";
            else if (LADSPA_IS_PORT_INPUT
                 (psDescriptor->PortDescriptors[lPortIndex]))
                collect_details +=":input ";
            else if (LADSPA_IS_PORT_OUTPUT
                 (psDescriptor->PortDescriptors[lPortIndex]))
                collect_details +=":output ";
            else 
                collect_details +="ERROR: NEITHER INPUT NOR OUTPUT";

            if (LADSPA_IS_PORT_INPUT
                (psDescriptor->PortDescriptors[lPortIndex])
                && LADSPA_IS_PORT_AUDIO
                (psDescriptor->PortDescriptors[lPortIndex])){
                inputports++;
            } else if (LADSPA_IS_PORT_OUTPUT
                (psDescriptor->PortDescriptors[lPortIndex])
                && LADSPA_IS_PORT_AUDIO
                (psDescriptor->PortDescriptors[lPortIndex])){
                outputports++;
            }
            
            iHintDescriptor 
              = psDescriptor->PortRangeHints[lPortIndex].HintDescriptor;

            if (LADSPA_IS_HINT_BOUNDED_BELOW(iHintDescriptor)
              || LADSPA_IS_HINT_BOUNDED_ABOVE(iHintDescriptor)) {
                if (LADSPA_IS_HINT_BOUNDED_BELOW(iHintDescriptor)) {
                    fBound = psDescriptor->PortRangeHints[lPortIndex].LowerBound;
                    if (LADSPA_IS_HINT_SAMPLE_RATE(iHintDescriptor) && fBound != 0) {
                        collect_details += sila::to_string(fBound);
                        collect_details +="*srate";
                    } else {
                        collect_details +=" Range: ";
                        collect_details += sila::to_string(fBound);
                    }
                } else collect_details +="...";
                collect_details +=" to ";
                if (LADSPA_IS_HINT_BOUNDED_ABOVE(iHintDescriptor)) {
                    fBound = psDescriptor->PortRangeHints[lPortIndex].UpperBound;
                    if (LADSPA_IS_HINT_SAMPLE_RATE(iHintDescriptor) && fBound != 0) {
                        collect_details += sila::to_string(fBound);
                        collect_details +="*srate";
                    } else {
                        collect_details += sila::to_string(fBound);
                    }
                } else collect_details +="...";
            }

            if (LADSPA_IS_HINT_TOGGLED(iHintDescriptor)) {
                if ((iHintDescriptor 
                   | LADSPA_HINT_DEFAULT_0
                   | LADSPA_HINT_DEFAULT_1)
                   != (LADSPA_HINT_TOGGLED 
                   | LADSPA_HINT_DEFAULT_0
                   | LADSPA_HINT_DEFAULT_1))
                    collect_details +=", ERROR: TOGGLED INCOMPATIBLE WITH OTHER HINT";
                else collect_details +=", toggled";
            }
              
            switch (iHintDescriptor & LADSPA_HINT_DEFAULT_MASK) {
                case LADSPA_HINT_DEFAULT_NONE:
                    break;
                case LADSPA_HINT_DEFAULT_MINIMUM:
                    fDefault = psDescriptor->PortRangeHints[lPortIndex].LowerBound;
                    if (LADSPA_IS_HINT_SAMPLE_RATE(iHintDescriptor) && fDefault != 0) {
                        collect_details += ", default ";
                        collect_details += sila::to_string(fDefault);
                        collect_details += "*srate";
                    } else {
                        collect_details += ", default ";
                        collect_details += sila::to_string(fDefault);
                    }
                    break;
                case LADSPA_HINT_DEFAULT_LOW:
                    if (LADSPA_IS_HINT_LOGARITHMIC(iHintDescriptor)) {
                        fDefault = exp(log(psDescriptor->PortRangeHints[lPortIndex].LowerBound) 
                        * 0.75
                        + log(psDescriptor->PortRangeHints[lPortIndex].UpperBound) 
                        * 0.25);
                    } else {
                        fDefault = (psDescriptor->PortRangeHints[lPortIndex].LowerBound
                        * 0.75
                        + psDescriptor->PortRangeHints[lPortIndex].UpperBound
                        * 0.25);
                    }
                    if (LADSPA_IS_HINT_SAMPLE_RATE(iHintDescriptor) && fDefault != 0) {
                        collect_details += ", default ";
                        collect_details += sila::to_string(fDefault);
                        collect_details += "*srate";
                    } else {
                        collect_details += ", default ";
                        collect_details += sila::to_string(fDefault);
                    }
                    break;
                case LADSPA_HINT_DEFAULT_MIDDLE:
                    if (LADSPA_IS_HINT_LOGARITHMIC(iHintDescriptor)) {
                        fDefault = sqrt(psDescriptor->PortRangeHints[lPortIndex].LowerBound
                        * psDescriptor->PortRangeHints[lPortIndex].UpperBound);
                    } else {
                        fDefault = 0.5 * (psDescriptor->PortRangeHints[lPortIndex].LowerBound
                        + psDescriptor->PortRangeHints[lPortIndex].UpperBound);
                    }
                    if (LADSPA_IS_HINT_SAMPLE_RATE(iHintDescriptor) && fDefault != 0)  {
                        collect_details += ", default ";
                        collect_details += sila::to_string(fDefault);
                        collect_details += "*srate";
                    } else{
                        collect_details += ", default ";
                        collect_details += sila::to_string(fDefault);
                    }
                    break;
                case LADSPA_HINT_DEFAULT_HIGH:
                    if (LADSPA_IS_HINT_LOGARITHMIC(iHintDescriptor)) {
                        fDefault = exp(log(psDescriptor->PortRangeHints[lPortIndex].LowerBound) 
                        * 0.25
                        + log(psDescriptor->PortRangeHints[lPortIndex].UpperBound) 
                        * 0.75);
                    } else {
                        fDefault = (psDescriptor->PortRangeHints[lPortIndex].LowerBound
                        * 0.25
                        + psDescriptor->PortRangeHints[lPortIndex].UpperBound
                        * 0.75);
                    }
                    if (LADSPA_IS_HINT_SAMPLE_RATE(iHintDescriptor) && fDefault != 0)  {
                        collect_details += ", default ";
                        collect_details += sila::to_string(fDefault);
                        collect_details += "*srate";
                    }else {
                        collect_details += ", default ";
                        collect_details += sila::to_string(fDefault);
                    }
                    break;
                case LADSPA_HINT_DEFAULT_MAXIMUM:
                    fDefault = psDescriptor->PortRangeHints[lPortIndex].UpperBound;
                    if (LADSPA_IS_HINT_SAMPLE_RATE(iHintDescriptor) && fDefault != 0)  {
                        collect_details += ", default ";
                        collect_details += sila::to_string(fDefault);
                        collect_details += "*srate";
                    } else {
                        collect_details += ", default ";
                        collect_details += sila::to_string(fDefault);
                    }
                    break;
                case LADSPA_HINT_DEFAULT_0:
                    collect_details +=", default 0";
                    break;
                case LADSPA_HINT_DEFAULT_1:
                    collect_details +=", default 1";
                    break;
                case LADSPA_HINT_DEFAULT_100:
                    collect_details +=", default 100";
                    break;
                case LADSPA_HINT_DEFAULT_440:
                    collect_details +=", default 440";
                    break;
                default:
                    collect_details +=", UNKNOWN DEFAULT CODE";
                    /* (Not necessarily an error - may be a newer version.) */
                    break;
            }
        
            if (LADSPA_IS_HINT_LOGARITHMIC(iHintDescriptor)) {
                collect_details +=" type logarithmic";
            }
            if (LADSPA_IS_HINT_INTEGER(iHintDescriptor)) {
                collect_details +=" type integer";
            }
                collect_details +="\n";
            }
            if(inputports == 1 && outputports == 1) {
                collect_first = " Type    :  MONO";
            } else if(inputports == 2 && outputports == 2) {
                collect_first = " Type    :  STEREO";
            } else if(inputports == 0 && outputports == 1) {
                collect_first = " Type    :  MONO SYNTH";
            } else if(inputports == 0 && outputports == 2) {
                collect_first = " Type    :  STEREO SYNTH";
            } else if(inputports < outputports && inputports) {
                collect_first = " Type    :  SPLITTER";
            } else if(inputports > outputports && outputports) {
                collect_first = " Type    :  MERGER";
            } else if(inputports == 0 && outputports == 0) {
                collect_first = " Type    :  NO AUDIO IN OR OUTPUT PORT FOUND";
            } else if(inputports == outputports ) {
                collect_first = " Type    :  MULTI CHANNEL";
            } else  {
                collect_first = " Type    :  MISC";
            }
        }
    }

    if (bVerbose) 
        collect_details +="\n";
    unloadLADSPAPluginLibrary(pvPluginHandle);
    collect_first += collect_details;
    details = collect_first.c_str();
    return details;
}

/*****************************************************************************/

void LadspaBrowser::make_menu(const LADSPA_Descriptor * psDescriptor, Glib::ustring str) {
    unsigned long lPortIndex;
    int inputports = 0;
    int outputports = 0;

    for (lPortIndex = 0; 
        lPortIndex < psDescriptor->PortCount; 
        lPortIndex++) {
        if  (LADSPA_IS_PORT_AUDIO(psDescriptor->PortDescriptors[lPortIndex]) && 
            LADSPA_IS_PORT_INPUT(psDescriptor->PortDescriptors[lPortIndex])) {
                inputports ++;
        } else if  (LADSPA_IS_PORT_AUDIO(psDescriptor->PortDescriptors[lPortIndex]) && 
            LADSPA_IS_PORT_OUTPUT(psDescriptor->PortDescriptors[lPortIndex])) {
                outputports ++;
        }
    }
    if(inputports == 1 && outputports == 1) {
        std::string mono_plug = "[";
        mono_plug += str.c_str();
        mono_plug += "] ";
        mono_plug += psDescriptor->Label;
        mono_plug += " ";
        mono_plug += sila::to_string(psDescriptor->UniqueID);
        plug_mono_list.push_back(mono_plug);
    } else if(inputports == 2 && outputports == 2) {
        std::string stereo_plug = "[";
        stereo_plug += str.c_str();
        stereo_plug += "] ";
        stereo_plug += psDescriptor->Label;
        stereo_plug += " ";
        stereo_plug += sila::to_string(psDescriptor->UniqueID);
        plug_stereo_list.push_back(stereo_plug);
    } else {
        std::string misc_plug = "[";
        misc_plug += str.c_str();
        misc_plug += "] ";
        misc_plug += psDescriptor->Label;
        misc_plug += " ";
        misc_plug += sila::to_string(psDescriptor->UniqueID);
        plug_misc_list.push_back(misc_plug);
    }
}

void LadspaBrowser::create_list() {
    Glib::ustring path;
    char * p = getenv ("LADSPA_PATH");
        if (p != NULL) {
            path = getenv ("LADSPA_PATH");
        } else { 
        setenv("LADSPA_PATH","/usr/lib/ladspa",0);
        setenv("LADSPA_PATH","/usr/local/lib/ladspa",0);
        path = getenv("LADSPA_PATH");
	}
    
    Glib::RefPtr<Gio::File> file = Gio::File::create_for_path(path);
    if (file->query_exists()) {
        Glib::RefPtr<Gio::FileEnumerator> child_enumeration =
              file->enumerate_children(G_FILE_ATTRIBUTE_STANDARD_NAME);
        std::vector<Glib::ustring> file_names;
        Glib::RefPtr<Gio::FileInfo> file_info;

        while ((file_info = child_enumeration->next_file())) {
            if (file_info->get_name().size() > 2) { // filefilter
             if (file_info->get_name().compare(file_info->get_name().size()-2, 2, "so") == 0)
                    file_names.push_back(file_info->get_name());
            }
        }
        // sort the vector
        std::sort(file_names.begin(), file_names.end());
        // clear the TreeView
        model->clear();
        // now populate the TreeView
        Gtk::TreeModel::Row row = *(model->append());
        
        for (unsigned int i = 0; i < file_names.size(); i++) {
            row[columns.name] = file_names[i];
            //fprintf(stderr, " %i %s \n", i,file_names[i].c_str());
            if (i != file_names.size() || i == 0) {
                
                Glib::ustring str = file_names[i];
                const char * plug = str.c_str();
                //const char* dat ;
                void * pvPluginHandle;
                LADSPA_Descriptor_Function pfDescriptorFunction;
                const LADSPA_Descriptor * psDescriptor;
                unsigned long lPluginIndex;
                pvPluginHandle = loadLADSPAPluginLibrary(plug);
                dlerror();
                pfDescriptorFunction 
                    = (LADSPA_Descriptor_Function)dlsym(pvPluginHandle, "ladspa_descriptor");
                if (!pfDescriptorFunction) {
                    const char * pcError = dlerror();
                    if (pcError) 
                      fprintf(stderr,
                          "Unable to find ladspa_descriptor() function in plugin file "
                          "\"%s\": %s.\n"
                          "Are you sure this is a LADSPA plugin file?\n", 
                          plug,
                          pcError);
                    return;
                }
                for (lPluginIndex = 0;; lPluginIndex++) {
                    psDescriptor = pfDescriptorFunction(lPluginIndex);
                    if (!psDescriptor)
                        break;
                    //dat = psDescriptor->Label;
                    Gtk::TreeModel::Row childrow = *(model->append(row.children()));
                    childrow[columns.name] = sila::to_string(psDescriptor->UniqueID);
                    
                    childrow[columns.label] = psDescriptor->Label;
                    make_menu(psDescriptor, str);
                    
                }
                // avoid appending a last empty row
                if (i != file_names.size()-1 || i == 0)
                    row = *(model->append());
                unloadLADSPAPluginLibrary(pvPluginHandle);
            }
        }
    }
}

void LadspaBrowser::on_pressed1_event() {
    static bool expand = true;
    if(expand) {
        treeview.expand_all();
        b1.set_label("Collaps_e Tree");
        b1.set_image (stck1);
        expand = false;
    } else {
        treeview.collapse_all();
        b1.set_label("  _Expand Tree");
        b1.set_image (stck2);
        expand = true;
    }
}

/*****************************************************************************/

LadspaBrowser::LadspaBrowser()
    : argv(),
    topBox(false, 0), 
    apply_button(Gtk::Stock::EXECUTE),
    quit_button(Gtk::Stock::QUIT),
    scrollWindow() {
    ++sila::Counter::get_instance()->getCount();
    set_default_size(450,450);
    set_title("Little Ladspa Host");
    set_decorated(true);
    set_resizable(true);
    set_gravity(Gdk::GRAVITY_SOUTH);
    m_refTextBuffer = Gtk::TextBuffer::create();
    model = Gtk::TreeStore::create(columns);
    treeview.set_headers_visible(false);
    treeview.set_model(model);
    treeview.set_name("ladspa_treeview");
    treeview.set_rules_hint(true);
    treeview.append_column("", columns.name);
    treeview.append_column("", columns.label);
    Gtk::RC::parse_string(
        "style 'treestyle'{ \n"
           " GtkTreeView::odd-row-color = 'gray94' \n"
           " GtkTreeView::even-row-color = 'gray100' \n"
           " GtkTreeView::allow-rules = 1 \n"
       " } \n"
       " widget '*ladspa_treeview*' style 'treestyle' \n");
    scrollWindow.add(treeview);
    scrollWindow.set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);
    topBox.pack_start(m_pane);
    m_pane.add1(scrollWindow);
    m_TextView.set_border_window_size(Gtk::TEXT_WINDOW_TOP, 20);
    m_TextView.set_editable(false);
    detailWindow.add(m_TextView);
    detailWindow.set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);
    m_pane.add2(detailWindow);
    m_pane.set_position(450);
    topBox.set_homogeneous(false);
    add(topBox);
    
    stck1.set(Gtk::Stock::JUSTIFY_FILL, Gtk::ICON_SIZE_BUTTON);
    stck2.set(Gtk::Stock::JUSTIFY_CENTER, Gtk::ICON_SIZE_BUTTON);
    
    b1.set_use_stock (true);
    b1.set_label ("  _Expand Tree");
    b1.set_image (stck2); 
    topBox.pack_end(ButtonBox,Gtk::PACK_SHRINK);
    ButtonBox.pack_end(apply_button,Gtk::PACK_SHRINK);
    ButtonBox.pack_end(b1,Gtk::PACK_SHRINK);
    ButtonBox.pack_start(quit_button,Gtk::PACK_SHRINK);

    b1.signal_activate().connect(
        sigc::mem_fun(*this, &LadspaBrowser::on_pressed1_event));
    b1.signal_pressed().connect(
        sigc::mem_fun(*this, &LadspaBrowser::on_pressed1_event));   
    apply_button.signal_clicked().connect(
        sigc::mem_fun(*this, &LadspaBrowser::on_execute));
    quit_button.signal_clicked().connect(
        sigc::mem_fun(*this, &LadspaBrowser::on_quit));
    treeview.signal_row_activated().connect(sigc::mem_fun(*this,
          &LadspaBrowser::on_treeview_row_activated) );
    treeview.signal_cursor_changed().connect(sigc::mem_fun(*this,
          &LadspaBrowser::on_cursor_changed) );
    create_list();
    fill_buffers("Select a Plugin");
}

/*****************************************************************************/

void LadspaBrowser::fill_buffers(Glib::ustring details)
{
  
    m_refTextBuffer->set_text(Glib::convert(details, "UTF-8", "ISO-8859-1"));
    m_TextView.set_buffer(m_refTextBuffer);
}

/*****************************************************************************/

void LadspaBrowser::on_treeview_row_activated(const Gtk::TreeModel::Path& path,
        Gtk::TreeViewColumn* /* column */) {
    // nothing to do
}

/*****************************************************************************/

void LadspaBrowser::on_cursor_changed() {
    Gtk::TreeModel::iterator iter = treeview.get_selection()->get_selected();
    if(iter) {
        Gtk::TreeModel::Row row = *iter; 
        Gtk::TreeModel::Row childrow = *(row.children().begin());
        Glib::ustring str = row[columns.name];
        Glib::ustring str1 = childrow[columns.name];
        const char * plug = str1.c_str();
        const char * lib = str.c_str();
        Glib::ustring det;
        if(strcmp(plug, lib) !=0) {
            det = analysePlugin(lib,NULL,0);
        } else {
            str = row[columns.label];
            if(strlen(str.c_str())) {
                plug = str.c_str();
                Gtk::TreeModel::Row parent = *(row.parent());
                str1 = parent[columns.name];
                const char * lib = str1.c_str();
                det = analysePlugin(lib,plug,1);
            } else {
                det = "NO LABEL OR UNIQUE-ID FOUND";
            }
        }
        fill_buffers(det);
    }
}

/*****************************************************************************/

void LadspaBrowser::on_execute() {
    Gtk::TreeModel::iterator iter = treeview.get_selection()->get_selected();
    if(iter) {
        Gtk::TreeModel::Row row = *iter; 
        Glib::ustring str = plhandle.libname = row[columns.name];
        Gtk::TreeModel::Row childrow = *(row.children().begin());
        Glib::ustring str1 = plhandle.UnicID = childrow[columns.name];
        const char * lib = str.c_str();
        const char * plug = str1.c_str();
        //const char * id = plhandle.UnicID.c_str();
        Glib::ustring det;
        if(strcmp(plug, lib) !=0) {
            str1 = plhandle.labelname = childrow[columns.label];
            plug = str1.c_str();
           // std::cout  << "load lib= "
           // << lib << " label= " << plug << " ID= " << id << std::endl;
        } else {
            str = plhandle.labelname = row[columns.label];
            if(strlen(str.c_str())) {
                plug = str.c_str();
                Gtk::TreeModel::Row parent = *(row.parent());
                str1 = plhandle.libname = parent[columns.name];
                lib = str1.c_str();
              //  std::cout  << "load lib="
              //  << lib << " label= " << plug << " ID= " << id << std::endl;
            }
        }
    }
    // create new instance of LadspaHost and run the given plugin
    sila_host::LadspaHost *ladspa_host = new sila_host::LadspaHost();
    argv[0] = NULL;
    argv[1] = const_cast<char*>(plhandle.libname.c_str());
    argv[2] = const_cast<char*>(plhandle.UnicID.c_str());
    int argc = 3;
    ladspa_host->sila_start(argc, argv);

}

/*****************************************************************************/

void LadspaBrowser::on_quit() {
    delete this;
    if (sila::Counter::get_instance()->getCount() <=0) {
        if(Gtk::Main::instance()->level()) Gtk::Main::quit();
    }
}

/*****************************************************************************/

void LadspaBrowser::show_browser() {
    show_all();    
}

/*****************************************************************************/

void LadspaBrowser::get_file_handle(Glib::ustring *plfile, int *plid) {
    *plfile = plhandle.libname;
    *plid = atoi(sila::to_string(plhandle.UnicID).c_str());
}

/*****************************************************************************/

LadspaBrowser::~LadspaBrowser() {
    --sila::Counter::get_instance()->getCount();
}

} //end namespace sila_browser
/*****************************************************************************/

