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

/**********************************************************************/
namespace sila {

static void set_default_value(LADSPA_PortRangeHintDescriptor hdescriptor,
                        float lower_bound, float upper_bound, float *dest) {
    float default_value = 0.0;
    if (LADSPA_IS_HINT_HAS_DEFAULT(hdescriptor)) {
        switch (hdescriptor & LADSPA_HINT_DEFAULT_MASK) {
         case LADSPA_HINT_DEFAULT_MINIMUM:
            default_value = lower_bound;
            break;
         case LADSPA_HINT_DEFAULT_LOW:
            if (LADSPA_IS_HINT_LOGARITHMIC(hdescriptor)) {
                default_value = exp(log(lower_bound) * 0.75 + log(upper_bound) * 0.25);
            } else {
                default_value = lower_bound * 0.75 + upper_bound * 0.25;
            }
            break;
         case LADSPA_HINT_DEFAULT_MIDDLE:
            if (LADSPA_IS_HINT_LOGARITHMIC(hdescriptor)) {
                default_value = exp(log(lower_bound) * 0.5 + log(upper_bound) * 0.5);
            } else {
                default_value = lower_bound * 0.5 + upper_bound * 0.5;
            }
            break;
         case LADSPA_HINT_DEFAULT_HIGH:
            if (LADSPA_IS_HINT_LOGARITHMIC(hdescriptor)) {
                default_value = exp(log(lower_bound) * 0.25 + log(upper_bound) * 0.75);
            } else {
                default_value = lower_bound * 0.25 + upper_bound * 0.75;
            }
            break;
         case LADSPA_HINT_DEFAULT_MAXIMUM:
            default_value = upper_bound;
            break;
         case LADSPA_HINT_DEFAULT_0:
            default_value = 0.0;
            break;
         case LADSPA_HINT_DEFAULT_1:
            default_value = 1.0;
            break;
         case LADSPA_HINT_DEFAULT_100:
            default_value = 100.0;
            break;
         case LADSPA_HINT_DEFAULT_440:
            default_value = 440.0;
            break;
        }
        *dest = default_value;
    }
}

/**********************************************************************/

class Slider : public Gtk::HScale {
 public:
    Slider(int port, LadspaHost& ladspa_host);
 private:
    void on_value_changed();
    float *dest;
};

Slider::Slider(int port,LadspaHost& ladspa_host)  {
    LADSPA_PortRangeHint hint = ladspa_host.ladspa_plug.descriptor->PortRangeHints[port];
    LADSPA_PortRangeHintDescriptor hdescriptor = hint.HintDescriptor;
    float lower_bound = hint.LowerBound;
    float upper_bound = hint.UpperBound;
    

    dest = &ladspa_host.ladspa_plug.cpv[port];

    if (LADSPA_IS_HINT_SAMPLE_RATE(hdescriptor)) {
        lower_bound *= ladspa_host.get_samplerate();
        upper_bound *= ladspa_host.get_samplerate();
    }
    if (LADSPA_IS_HINT_LOGARITHMIC(hdescriptor))
    {
      if (lower_bound < FLT_EPSILON)
          lower_bound = FLT_EPSILON;
    }

    if (LADSPA_IS_HINT_BOUNDED_BELOW(hdescriptor) &&
        LADSPA_IS_HINT_BOUNDED_ABOVE(hdescriptor)) {
        set_range(lower_bound, upper_bound);
    } else if (LADSPA_IS_HINT_BOUNDED_BELOW(hdescriptor)) {
        set_range(lower_bound, 1.0);
    } else if (LADSPA_IS_HINT_BOUNDED_ABOVE(hdescriptor)) {
        set_range(0.0, upper_bound);
    } else {
        set_range(-1.0, 1.0);
    }

    if (LADSPA_IS_HINT_TOGGLED(hdescriptor)) {
        set_range(0.0, 1.0);
        set_increments(1.0, 1.0);
    }

    if (LADSPA_IS_HINT_INTEGER(hdescriptor)) {
        set_increments(1.0, 1.0);
        set_digits(0);
    } else {
        set_increments(0.05, 0.1);
        set_digits(2);
    }
    set_default_value(hdescriptor, lower_bound, upper_bound, dest);
   // here we can set initial values if we wish like that
   /* if(port == 13 && ladspa_host.ladspa_plug.descriptor->UniqueID == 34049) {
        *dest = 2990.7;
    } else if (port == 12 && ladspa_host.ladspa_plug.descriptor->UniqueID == 33918) {
        *dest = 3556.56;
    }*/
    set_value(*dest);
}

void Slider::on_value_changed() {
    *dest = get_value();
}

/**********************************************************************/

class TButton : public Gtk::ToggleButton {
 public:
    TButton(int port, LadspaHost& ladspa_host);
 private:
    void on_toggled();
    float *dest;
};

TButton::TButton(int port, LadspaHost& ladspa_host) {
    LADSPA_PortRangeHint hint = ladspa_host.ladspa_plug.descriptor->PortRangeHints[port];
    LADSPA_PortRangeHintDescriptor hdescriptor = hint.HintDescriptor;
    set_label(ladspa_host.ladspa_plug.descriptor->PortNames[port]);

    float default_value;
    dest = &ladspa_host.ladspa_plug.cpv[port];

    if (LADSPA_IS_HINT_HAS_DEFAULT(hdescriptor)) {
        switch (hdescriptor & LADSPA_HINT_DEFAULT_MASK) {
         case LADSPA_HINT_DEFAULT_0:
            default_value = 0.0;
            set_active(false);
            break;
         case LADSPA_HINT_DEFAULT_1:
            default_value = 1.0;
            set_active(true);
            break;
         default:
            default_value = 0.0;
            set_active(false);
            break;
        }
        *dest = default_value;
    }
}

void TButton::on_toggled() {
    if(get_active()) *dest = 1.0;
    else *dest = 0.0;
}

/**********************************************************************/

class Spiner : public Gtk::SpinButton {
 public:
    Spiner(int port, LadspaHost& ladspa_host);
 private:
    void on_value_changed();
    float *dest;
};

Spiner::Spiner(int port, LadspaHost& ladspa_host) {
    LADSPA_PortRangeHint hint = ladspa_host.ladspa_plug.descriptor->PortRangeHints[port];
    LADSPA_PortRangeHintDescriptor hdescriptor = hint.HintDescriptor;
    float lower_bound = hint.LowerBound;
    float upper_bound = hint.UpperBound;

    dest = &ladspa_host.ladspa_plug.cpv[port];

    if (LADSPA_IS_HINT_SAMPLE_RATE(hdescriptor)) {
        lower_bound *= ladspa_host.get_samplerate();
        upper_bound *= ladspa_host.get_samplerate();
    }
    if (LADSPA_IS_HINT_LOGARITHMIC(hdescriptor))
    {
      if (lower_bound < FLT_EPSILON)
          lower_bound = FLT_EPSILON;
    }

    if (LADSPA_IS_HINT_BOUNDED_BELOW(hdescriptor) &&
        LADSPA_IS_HINT_BOUNDED_ABOVE(hdescriptor)) {
        set_range(lower_bound, upper_bound);
    } else if (LADSPA_IS_HINT_BOUNDED_BELOW(hdescriptor)) {
        set_range(lower_bound, 1.0);
    } else if (LADSPA_IS_HINT_BOUNDED_ABOVE(hdescriptor)) {
        set_range(0.0, upper_bound);
    } else {
        set_range(-1.0, 1.0);
    }

    set_increments(1.0, 1.0);
    set_digits(0);

    set_default_value(hdescriptor, lower_bound, upper_bound, dest);
   // here we can set initial values if we wish like that
   /* if(port == 13 && ladspa_host.ladspa_plug.descriptor->UniqueID == 34049) {
        *dest = 2990;
    } else if (port == 12 && ladspa_host.ladspa_plug.descriptor->UniqueID == 33918) {
        *dest = 3556;
    }*/
    set_value(*dest);
}

void Spiner::on_value_changed() {
    *dest = get_value();
}

/**********************************************************************/

Gtk::Window *LadspaHost::create_widgets() {
	LadspaHost& ladspa_host = *static_cast<LadspaHost*>(this);
    Gtk::VBox *main_box = Gtk::manage(new Gtk::VBox);
    Gtk::VBox *controller_box = Gtk::manage(new Gtk::VBox);
 
    for (int i = 0; i < (int)ladspa_host.ladspa_plug.descriptor->PortCount; i++) {
        LADSPA_PortRangeHint hint = ladspa_host.ladspa_plug.descriptor->PortRangeHints[i];
        LADSPA_PortRangeHintDescriptor hdescriptor = hint.HintDescriptor;
        if (LADSPA_IS_PORT_INPUT(ladspa_host.ladspa_plug.descriptor->PortDescriptors[i]) &&
            LADSPA_IS_PORT_CONTROL(ladspa_host.ladspa_plug.descriptor->PortDescriptors[i])) {
            if (LADSPA_IS_HINT_TOGGLED(hdescriptor)) {
                Gtk::HBox *box = Gtk::manage(new Gtk::HBox);
                box->set_spacing(10);
                box->pack_start(*Gtk::manage(new TButton( i,ladspa_host)),
                                Gtk::PACK_SHRINK);
                box->pack_start(*Gtk::manage(new Gtk::HBox), Gtk::PACK_EXPAND_WIDGET);
                controller_box->pack_start(*box, Gtk::PACK_SHRINK);
            } else if (LADSPA_IS_HINT_INTEGER(hdescriptor)) {
                Gtk::HBox *box = Gtk::manage(new Gtk::HBox);
                box->set_spacing(10);
                box->pack_start(*Gtk::manage(new Gtk::Label(ladspa_host.ladspa_plug.descriptor->PortNames[i]
                                )), Gtk::PACK_SHRINK);
                box->pack_start(*Gtk::manage(new Spiner(i,ladspa_host)),
                                Gtk::PACK_SHRINK);
                box->pack_start(*Gtk::manage(new Gtk::HBox), Gtk::PACK_EXPAND_WIDGET);
                controller_box->pack_start(*box, Gtk::PACK_SHRINK);
            } else {
                Gtk::HBox *box = Gtk::manage(new Gtk::HBox);
                box->set_spacing(10);
                controller_box->pack_start(*Gtk::manage(new Gtk::Label(ladspa_host.ladspa_plug.descriptor->PortNames[i]
                                )), Gtk::PACK_EXPAND_WIDGET);
                box->pack_start(*Gtk::manage(new Slider(i,ladspa_host)),
                                Gtk::PACK_EXPAND_WIDGET);
                controller_box->pack_start(*box, Gtk::PACK_SHRINK);
            }
            Gtk::HSeparator *sep = Gtk::manage(new Gtk::HSeparator);
            controller_box->pack_start(*sep, Gtk::PACK_EXPAND_WIDGET);
        }
    }
    if (ladspa_plug.cnp > 12) {
        Gtk::ScrolledWindow *scrolled_window = Gtk::manage(new Gtk::ScrolledWindow);
        scrolled_window->set_policy(Gtk::POLICY_NEVER, Gtk::POLICY_ALWAYS);
        scrolled_window->set_shadow_type(Gtk::SHADOW_NONE);
        scrolled_window->add(*controller_box);
        scrolled_window->set_size_request(400, 300);
        main_box->pack_start(*scrolled_window, Gtk::PACK_EXPAND_WIDGET);
    } else { 
        main_box->pack_start(*controller_box, Gtk::PACK_EXPAND_WIDGET);
    }
    return((Gtk::Window*)main_box);
}

} //end namespace sila
/*****************************************************************************/
