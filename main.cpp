#include <gtkmm.h>
#include <iostream>
#include <cstdlib>
#include <vector>
#include <math.h>
#include <experimental/filesystem>

#include "text_editor.h"

//The power of two (Book)

//To implement zoom by scrolling you have to keep the pixel under the cursor
//at the same location while changing the scale.
//To achieve this you have to calculate the new corner coordinates relative to the pixel under the pointer

namespace fs = std::experimental::filesystem;
using namespace Gtk;

class AdaptiveImage : public DrawingArea{
private:
    Glib::RefPtr<Gdk::Pixbuf> original_image , shown_image;    
    Glib::RefPtr<Adjustment> x , y , scale;
    fs::path file;
    bool emtpy = true;
    bool auto_scale = true;
    bool file_exists;
    struct{
        int x_init , y_init , x_final , y_final , dragging=false;
    }mouse;
    struct{
        double last_scale;
        bool scrolling = false;
        bool need_save = false;
    }memory;
    void setup(){
        x = Adjustment::create(0 , -1 , 0 , 0.001);
        y = Adjustment::create(0 , -1 , 0 , 0.001);
        scale = Adjustment::create(1 , 0.1 , 3 , 0.1 , 0.1 , 0.1);

        scale->signal_value_changed().connect([&](){
            if(!memory.scrolling){
                adjust_to_pointer_position(memory.last_scale , get_allocation().get_width()/2 , get_allocation().get_height()/2);
            }else{
                memory.scrolling = false;
            }
            memory.last_scale = scale->get_value();
            queue_draw();
        });
        x->signal_value_changed().connect([&](){queue_draw();});
        y->signal_value_changed().connect([&](){queue_draw();});
        memory.last_scale = scale->get_value();

        add_events(Gdk::EventMask::ALL_EVENTS_MASK);
        signal_event().connect([&](GdkEvent* e){
            if(e->type == GdkEventType::GDK_SCROLL){
                zoom_by_scrolling(e->scroll.delta_x , e->scroll.delta_y , e->scroll.x , e->scroll.y);
            }else if(e->type == GdkEventType::GDK_BUTTON_RELEASE){
                mouse.dragging = false;
            }else if(e->type == GdkEventType::GDK_BUTTON_PRESS){
                mouse.dragging = true;
                mouse.x_init = e->motion.x;
                mouse.x_init = e->motion.y;
            }else if(mouse.dragging){
                x->set_value(x->get_value()+(e->motion.x - mouse.x_init));
                y->set_value(y->get_value()+(e->motion.y - mouse.y_init));
                
                mouse.x_init = e->motion.x;
                mouse.y_init = e->motion.y;
                //std::cout << e->motion.x - mouse.x_init << " " << e->motion.y - mouse.y_init << std::endl;
            }

            return false;
        });

        signal_button_press_event().connect([&](GdkEventButton* e){
            mouse.x_init = e->x;
            mouse.y_init = e->y;

            auto cursor = Gdk::Cursor::create(Gdk::CursorType::CROSS);
            get_parent_window()->set_cursor(cursor);        
            return false;
        });
        signal_button_release_event().connect([&](GdkEventButton* e){
            mouse.x_final = e->x;
            mouse.y_final = e->y;
            
            x->set_value(x->get_value()+(mouse.x_final - mouse.x_init));
            y->set_value(y->get_value()+(mouse.y_final - mouse.y_init));

            auto cursor = Gdk::Cursor::create(Gdk::CursorType::ARROW);
            get_parent_window()->set_cursor(cursor); 
            return false;
        });
    }
public:
    AdaptiveImage(Glib::RefPtr<Gdk::Pixbuf> pixbuf):original_image(pixbuf){
        file_exists = false;
        setup();
    }
    AdaptiveImage(std::string file):original_image(Gdk::Pixbuf::create_from_file(file)) , file(file){
        file_exists = true;
        setup();
    }

    void rotate_left(){
        original_image = original_image->rotate_simple(Gdk::PixbufRotation::PIXBUF_ROTATE_COUNTERCLOCKWISE);
        memory.need_save = true;
        queue_draw();
    }

    void rotate_right(){
        original_image = original_image->rotate_simple(Gdk::PixbufRotation::PIXBUF_ROTATE_CLOCKWISE);
        memory.need_save = true;
        queue_draw();
    }

    void set_file_save(std::string file){
        this->file = file;
        file_exists = true;
    }

    void save(){
        std::string type = file.extension().string();
        clear(0 , type);
        if(file_exists){
            original_image->save(file , type);
            memory.need_save = false;
        }
    }
    void save_as(fs::path dir, std::string format){
            original_image->save(dir , format);
    }
    bool need_save(){
        return memory.need_save;
    }

    //The craziest function ever made
    void zoom_by_scrolling(double delta_x , double delta_y , double pointer_x , double pointer_y){
        memory.scrolling = true;
        int x1 = pointer_x - x->get_value();
        int x2 = original_image->get_width()*scale->get_value() - x1;
        int init_width = original_image->get_width()*scale->get_value();

        int y1 = pointer_y - y->get_value();
        int y2 = original_image->get_height()*scale->get_value() - y1;
        int init_height = original_image->get_height()*scale->get_value();

        set_auto_scale(false);
        scale->set_value(scale->get_value()-delta_y/10);

        int final_width = original_image->get_width()*scale->get_value();
        int delta_width = final_width - init_width;

        int delta_x1 = (x1*x2 + x1*delta_width - x2*x1)/(x2+x1);
        int delta_x2 = delta_width - delta_x1;
        x->set_value(x->get_value()-delta_x1);

        int final_height = original_image->get_height()*scale->get_value();
        int delta_height = final_height - init_height;

        int delta_y1 = (y1*y2 + y1*delta_height - y2*y1)/(y2+y1);
        int delta_y2 = delta_height - delta_y1;
        y->set_value(y->get_value()-delta_y1);
        
        queue_draw();
    }

    void adjust_to_pointer_position(double last_scale , double pointer_x , double pointer_y){
        //std::cout << pointer_x << " " << pointer_y << std::endl;
        int x1 = pointer_x - x->get_value();
        int x2 = original_image->get_width()*last_scale - x1;
        int init_width = original_image->get_width()*last_scale;

        int y1 = pointer_y - y->get_value();
        int y2 = original_image->get_height()*last_scale - y1;
        int init_height = original_image->get_height()*last_scale;

        //set_auto_scale(false);
        //scale->set_value(new_scale);

        int final_width = original_image->get_width()*scale->get_value();
        int delta_width = final_width - init_width;

        int delta_x1 = (x1*x2 + x1*delta_width - x2*x1)/(x2+x1);
        int delta_x2 = delta_width - delta_x1;
        x->set_value(x->get_value()-delta_x1);

        int final_height = original_image->get_height()*scale->get_value();
        int delta_height = final_height - init_height;

        int delta_y1 = (y1*y2 + y1*delta_height - y2*y1)/(y2+y1);
        int delta_y2 = delta_height - delta_y1;
        y->set_value(y->get_value()-delta_y1);

        queue_draw();
    }

    void set_auto_scale(bool condition = true){auto_scale = condition;}
    bool get_auto_scale(){return auto_scale;}

    void set_scale(double percentage){scale->set_value(percentage);}
    double get_scale(){return scale->get_value();}

    void set_x(int x){this->x->set_value(x);}
    void set_y(int y){this->y->set_value(y);}

    const Glib::RefPtr<Gdk::Pixbuf> get_pixbuf(){return original_image;}
    void set_from_file(std::string file){
        if(fs::exists(file)){
            original_image = Gdk::Pixbuf::create_from_file(file);
        }
    }
    void set_from_pixbuf(Glib::RefPtr<Gdk::Pixbuf> pixbuf){
        original_image = pixbuf;
    }

    int get_x(){return x->get_value();}
    int get_y(){return y->get_value();}

    Glib::RefPtr<Adjustment> get_scale_adjustment(){return scale;}
    Glib::RefPtr<Adjustment> get_x_adjustment(){return x;}
    Glib::RefPtr<Adjustment> get_y_adjustment(){return y;}
    

    virtual bool on_draw(const Cairo::RefPtr<Cairo::Context> &context){
        int width = original_image->get_width() , height = original_image->get_height();
        const int allocated_width = this->get_allocation().get_width() , allocated_height = this->get_allocation().get_height();

        x->set_page_size(allocated_width);
        y->set_page_size(allocated_height);


            if(auto_scale){
                if((width < allocated_width && height < allocated_height)){
                    //shown_image = original_image;
                    scale->set_value(1.0);
                }else{
                    double value = double(allocated_width) / width;
                    if(value * height > allocated_height){
                        value = double(allocated_height) / height;
                    }
                    scale->set_value(value);
                }
            }
            shown_image = original_image->scale_simple(scale->get_value()*width , scale->get_value()*height , Gdk::InterpType::INTERP_BILINEAR);

        x->set_lower([&](){
            if(width*scale->get_value() <= allocated_width){
                x->set_upper(0);
                return 0.0;
            }else{
                x->set_upper(allocated_width);
                return allocated_width - width*scale->get_value(); //- x->get_page_size();
            }}());

        y->set_lower([&](){
            if(height*scale->get_value() <= allocated_height){
                y->set_upper(0);
                return 0.0;
            }else{
                y->set_upper(allocated_height);
                return allocated_height - height*scale->get_value();// - y->get_page_size();
            }
        }());
        if(x->get_value() < x->get_lower()){x->set_value(x->get_lower());}
        if(y->get_value() < y->get_lower()){y->set_value(y->get_lower());}

        if(int(x->get_lower()==0) || int(y->get_lower()==0)){ //If image is smaller than or equal to allocated size
            Gdk::Cairo::set_source_pixbuf(context , shown_image , (allocated_width-width*scale->get_value())/2 , (allocated_height-height*scale->get_value())/2);
            //make it centered
        }else{
            Gdk::Cairo::set_source_pixbuf(context , shown_image , x->get_value() , y->get_value());
            //otherwise use the user requested position
        }
        context->paint();
        return false;   
    }
};

struct{
    bool auto_save = true;
}settings;



std::string get_arguments(int argc , char* argv[]){
    std::string text = "";
    if(argc < 2){
        return "";
    }
    for(int i=1 ; i<argc ; i++){
        text.append(argv[i]);
    }
    return text;
}

int main(int argc , char* argv[]){

    fs::path file = get_arguments(argc , argv);
    if(argc < 2){
        std::cout << "usage:\n view [path]" << std::endl;
        return 1;
    }
    if(!fs::exists(file)){
        std::cout << file << " Not Found" << std::endl;
        return 1;
    }


    auto app = Application::create();
    auto screen = Gdk::Screen::get_default();

    Window window;  
    window.set_size_request(-1 , 300);
    window.set_default_size(600 , 400);
    window.set_icon_name("image-viewer");

    HeaderBar titlebar;
    window.set_titlebar(titlebar);
    titlebar.set_title(file.filename().string());
    titlebar.set_show_close_button(true);

    Button menu_button , zoom_button , edit_button;
    menu_button.set_image_from_icon_name("application-menu");
    zoom_button.set_image_from_icon_name("zoom");
    edit_button.set_image_from_icon_name("document-edit");

    Button auto_button , rotate_left_button , rotate_right_button;
    auto_button.set_image_from_icon_name("fitbest");
    rotate_left_button.set_image_from_icon_name("object-rotate-left");
    rotate_right_button.set_image_from_icon_name("object-rotate-right");

    titlebar.pack_start(menu_button);
    titlebar.pack_start(zoom_button);
    titlebar.pack_start(auto_button);
    titlebar.pack_start(rotate_left_button);
    titlebar.pack_start(rotate_right_button);
    titlebar.pack_end(edit_button);

    ModelButton save_item , save_as_item , preferences_item , details_item , about_item;
    details_item.set_label("Details");
    about_item.set_label("About");
    save_item.set_label("Save");
    save_as_item.set_label("Save as..");
    preferences_item.set_label("Preferences");

    VBox menu_box;
    menu_box.set_border_width(5);
    menu_box.pack_start(save_item , PACK_SHRINK);
    menu_box.pack_start(save_as_item , PACK_EXPAND_WIDGET);
    menu_box.pack_start(details_item , PACK_EXPAND_WIDGET);
    menu_box.pack_start(preferences_item , PACK_EXPAND_WIDGET);
    menu_box.pack_start(about_item , PACK_EXPAND_WIDGET);

    Popover popover;
    popover.set_relative_to(menu_button);
    popover.add(menu_box);
    popover.show_all_children();

    menu_button.signal_clicked().connect([&](){
        popover.popup();
    });

    Scale zoom_scale;
    zoom_scale.set_range(0.1 , 3);
    zoom_scale.add_mark(1 , PositionType::POS_BOTTOM , "");
    zoom_scale.set_size_request(150);
    zoom_scale.property_margin() = 5;

    Popover scroll_popover;
    scroll_popover.add(zoom_scale);
    scroll_popover.set_relative_to(zoom_button);
    scroll_popover.show_all_children();

    zoom_button.signal_clicked().connect([&](){
        scroll_popover.popup();
    });

    AdaptiveImage img(file);
    img.signal_button_press_event().connect([&](GdkEventButton* e){
        static bool fullscreen = false;
        if(e->type == Gdk::EventType::DOUBLE_BUTTON_PRESS){
            if(!fullscreen){
                window.fullscreen();
                fullscreen = true;
            }else{
                window.unfullscreen();
                fullscreen = false;
            }
        }
        return false;
    });

    HBox hbox;
    VScrollbar v;
    hbox.pack_start(img , PACK_EXPAND_WIDGET);
    hbox.pack_end(v , PACK_SHRINK);

    VBox vbox;
    HScrollbar h;
    vbox.pack_start(hbox , PACK_EXPAND_WIDGET);
    vbox.pack_end(h , PACK_SHRINK);

    window.add(vbox);

    h.set_adjustment(img.get_x_adjustment());
    v.set_adjustment(img.get_y_adjustment());
    h.set_inverted();
    v.set_inverted();

    v.get_adjustment()->property_lower().signal_changed().connect([&](){
        if(int(v.get_adjustment()->get_lower()) == 0){
            v.hide();
        }else{
            v.show();
        }
    });

    h.get_adjustment()->property_lower().signal_changed().connect([&](){
        if(int(h.get_adjustment()->get_lower()) == 0){
            h.hide();
        }else{
            h.show();
        }
    });

    auto_button.signal_clicked().connect([&](){
        img.set_auto_scale(true);
        img.queue_draw();
    });

    zoom_scale.set_adjustment(img.get_scale_adjustment());

    zoom_button.signal_clicked().connect([&](){
        img.set_auto_scale(false);
    });

    rotate_right_button.signal_clicked().connect([&](){
        img.rotate_right();
        if(settings.auto_save){
            img.save();
        }
    });

    rotate_left_button.signal_clicked().connect([&](){
        img.rotate_left();
        if(settings.auto_save){
            img.save();
        }
    });

    save_item.signal_clicked().connect([&](){
        img.save();
    });

    //-------------------------Save as dialog----------------------------------------------
    save_as_item.signal_clicked().connect([&](){
        FileChooserDialog fcd("Save as" , FILE_CHOOSER_ACTION_SAVE);
        //HeaderBar fcd_titlebar;
        //fcd_titlebar.set_title("Save as");
        //fcd.set_titlebar(fcd_titlebar);

        auto formats = Gdk::Pixbuf::get_formats();
        std::vector<Glib::RefPtr<FileFilter>> filters; 
        for(auto format:formats){
            auto filter = FileFilter::create();
            filter->set_name(format.get_name());
            auto mimes = format.get_mime_types();
            for(auto mime : mimes){
                filter->add_mime_type(mime);
            }
            filters.push_back(filter);
        }

        for(auto filter : filters){
            fcd.add_filter(filter);
        }

        fcd.add_button("Cancel" , RESPONSE_CANCEL);
        fcd.add_button("Save" , RESPONSE_OK);
        //fcd.show_all_children();

        int response = fcd.run();

        if(response == RESPONSE_OK){
            img.save_as(fcd.get_file()->get_path() , fcd.get_filter()->get_name());
        }else{
            fcd.hide();
        }
    });
    //-----------------------------------------------------------------
    //----------------Details Window-----------------------------------
    Window details_window;
    details_window.set_resizable(false);

    HeaderBar details_window_title_bar;
    details_window.set_titlebar(details_window_title_bar);
    details_window_title_bar.set_title("Details");
    details_window_title_bar.set_show_close_button(true);

    VBox details_box;
    details_box.set_border_width(10);
    Label lname("Name: "+file.filename().string()) 
        , lpath("Path: "+file.string())
        , ldimensions("Dimensions: "+std::to_string(img.get_pixbuf()->get_width())+"x"+std::to_string(img.get_pixbuf()->get_height()))
        , lsize("Size: "+std::to_string(fs::file_size(file))+" Bytes") 
        , ltype("Type: " +file.extension().string());

    lname.property_halign() = Gtk::Align::ALIGN_START;
    lpath.property_halign() = Gtk::Align::ALIGN_START;
    ldimensions.property_halign() = Gtk::Align::ALIGN_START;
    lsize.property_halign() = Gtk::Align::ALIGN_START;
    ltype.property_halign() = Gtk::Align::ALIGN_START;

    details_box.pack_start(lname , PACK_SHRINK);
    details_box.pack_start(lpath , PACK_SHRINK);
    details_box.pack_start(ldimensions , PACK_SHRINK);
    details_box.pack_start(lsize , PACK_SHRINK);
    details_box.pack_start(ltype , PACK_SHRINK);

    details_window.add(details_box);
    details_window.show_all_children();

    details_item.signal_clicked().connect([&](){
        details_window.show();    
    });

    //-----------------------------------------------------------------

    //-------------------Preferences Dialog----------------------------
    Window preferences_window;
    preferences_window.set_resizable(false);

    HeaderBar pref_titlebar;
    preferences_window.set_titlebar(pref_titlebar);
    pref_titlebar.set_title("Preferences");
    pref_titlebar.set_show_close_button(true);

    VBox preferences_box;
    preferences_box.property_margin() = 5;
    preferences_window.add(preferences_box);
    
    HBox auto_save_box;
    preferences_box.pack_start(auto_save_box , PACK_SHRINK);
    Label auto_save_label;
    Switch auto_save_switch;
    auto_save_box.pack_start(auto_save_label , PACK_EXPAND_PADDING);
    auto_save_box.pack_start(auto_save_switch , PACK_EXPAND_PADDING);

    auto_save_switch.set_state(settings.auto_save);
    auto_save_label.set_label("Auto Save");
    auto_save_switch.signal_state_changed().connect([&](StateType type){
        settings.auto_save = auto_save_switch.get_state();
    });

    preferences_window.set_transient_for(window);

    preferences_item.signal_button_release_event().connect([&](GdkEventButton *e){
        preferences_window.show();
        return true;
    });

    preferences_window.show_all_children();

    //-----------------------------------------------------------------

    //-------------------confirm before exit---------------------------
    MessageDialog askd("Do you want to save before exiting?" , false , MESSAGE_QUESTION , BUTTONS_YES_NO);
    askd.add_button("Cancel" , RESPONSE_CANCEL);
    askd.set_transient_for(window);
    window.signal_delete_event().connect([&](const GdkEventAny* e){
        if(img.need_save()){
            int response = askd.run();
            if(response == RESPONSE_YES){
                askd.hide();
                img.save();
                return false;
            }else if(response == RESPONSE_NO){
                askd.hide();
                return false;
            }else if(response == RESPONSE_CANCEL){
                askd.hide();
                return true;
            }
        }else{
            return false;
        }
        return false;
    });
    //-----------------------------------------------------------------


    //-------------------About Dialog----------------------------------
    AboutDialog aboutd;
    aboutd.set_transient_for(window);

    HeaderBar about_titlebar;
    aboutd.set_titlebar(about_titlebar);
    about_titlebar.set_title("About Image Viewer");

    aboutd.set_program_name("Image Viewer");
    aboutd.set_logo_icon_name("image-viewer");
    aboutd.set_license_type(License::LICENSE_GPL_3_0);

    std::vector<Glib::ustring> authors;
    authors.push_back("Hamza Algohary");
    aboutd.set_authors(authors);

    about_item.signal_clicked().connect([&](){
        int response = aboutd.run();
            aboutd.hide();
    });
    aboutd.show_all_children();
    
    //-----------------------------------------------------------------
    window.show_all_children();
    return app->run(window);
}

    //HBox main_box;
    //window.add(main_box);
    //Overlay main_overlay;
    //main_overlay.remove()
    //window.add(main_overlay);
    //main_overlay.add(main_box);
    /*Picture picture;
    picture.set_from_file(file);*/

    /*picture.add_events(Gdk::EventMask::BUTTON_PRESS_MASK);
    picture.signal_button_press_event().connect([&](GdkEventButton* e){
        const int fullscreen=0 , not_fullscreen=1;
        static int state = not_fullscreen;
        if(e->type == Gdk::EventType::DOUBLE_BUTTON_PRESS){
            if(state==not_fullscreen){
                window.fullscreen();
                state = fullscreen;
            }else if(state == fullscreen){
                window.unfullscreen();
                state = not_fullscreen;
            }
        }
        return true;
    });*/

    /*Overlay main_overlay;
    main_box.pack_start(main_overlay);

    main_overlay.add(picture);
    Scrollbar h_scrollbar;
    VScrollbar v_scrollbar;
    h_scrollbar.property_halign() = Align::ALIGN_FILL;
    h_scrollbar.property_valign() = Align::ALIGN_END;
    v_scrollbar.property_halign() = Align::ALIGN_END;
    v_scrollbar.property_valign() = Align::ALIGN_FILL;

    main_overlay.add_overlay(h_scrollbar);
    main_overlay.add_overlay(v_scrollbar);

    h_scrollbar.set_range(-1000 , 1000);
    v_scrollbar.set_range(-1000 , 1000);
    h_scrollbar.set_value(0);
    v_scrollbar.set_value(0);

    h_scrollbar.signal_change_value().connect([&](ScrollType type , double value){
        picture.set_x_center(((value*-1)/1000)*picture.get_width());
        return false;
    });
    v_scrollbar.signal_change_value().connect([&](ScrollType type , double value){
        picture.set_y_center(((value*-1)/1000)*picture.get_height());
        return false;
    });*/
    //picture.set_h_adjustment(h_scrollbar.get_adjustment());
    //picture.set_v_adjustment(v_scrollbar.get_adjustment());

    //window.signal_size_allocate().connect([&](Allocation &a){picture.update_adjustments(); picture.queue_draw();});
    /*Scrollbar hscale;
    picture.set_x_range_widget(hscale);
    //hscale.set_range(-100 , 0);
    //hscale.set_inverted(true);
    hscale.set_halign(Align::ALIGN_FILL);
    hscale.set_valign(Align::ALIGN_END);
    hscale.set_margin_left(10);
    hscale.set_margin_right(20);
    //hscale.set_size_request(100);

    VScrollbar vscale;
    picture.set_x_range_widget(vscale);
    //vscale.set_range(-100 , 0);
    //vscale.set_inverted(true);
    vscale.set_halign(Align::ALIGN_END);
    vscale.set_valign(Align::ALIGN_FILL);
    vscale.set_margin_top(10);
    vscale.set_margin_bottom(20);
    //vscale.set_size_request(-1 , 100);*/

    /*main_overlay.add_overlay(hscale);
    main_overlay.add_overlay(vscale);
    */
    /*zoom_scale.signal_change_value().connect([&](ScrollType type , double value){
        /*if(zoom_scale.get_value() == 0){
            //hscale.hide();
            //vscale.hide();
        }else{
            //hscale.show();
            //vscale.show();
        }*/
        //img.set_scale(value);
        //picture.do_zoom(zoom_scale.get_value());

    /*    return true;
    });*/

    /*hscale.signal_change_value().connect([&](ScrollType type , double value){
        //picture.set_xoffset_percentage(hscale.get_value());
        picture.set_x_center(hscale.get_value());
        return true;
    });

    vscale.signal_change_value().connect([&](ScrollType type , double value){
        //picture.set_yoffset_percentage(vscale.get_value());
        picture.set_y_center(vscale.get_value());
        return true;
    });*/

/*class Picture : public DrawingArea{
public:
    Glib::RefPtr<Gdk::Pixbuf> original_image;
    Glib::RefPtr<Gdk::Pixbuf> image;
    //Glib::RefPtr<Gtk::Range> x_scale , y_scale;
    //Range* x_scale , y_scale;
    Glib::RefPtr<Adjustment> v_adj , h_adj;
    bool h_empty = true , v_empty = true;
    int allocated_width , allocated_height;
    int scaled_width , scaled_height;
    double scaling_factor = 1;
    double x_center = 0 , y_center = 0;

    bool empty=true;
    bool no_size_change = true;
    Picture(){}

    void set_dimensions(){//(int &width , int &height , int available_width , int available_height){
        scaled_width = allocated_width;
        scaled_height = (scaled_width*original_image->get_height())/original_image->get_width();
        if(scaled_height > allocated_height){
            scaled_height = allocated_height;
            scaled_width = (scaled_height*original_image->get_width())/original_image->get_height();
        }

        scaled_width *= scaling_factor;
        scaled_height *= scaling_factor;
    }

    void do_zoom(double scaling_factor){
        this->scaling_factor = scaling_factor;
        queue_draw();
        //update_adjustments();
    }

    /*void update_adjustments(){
        if(!h_empty){
            h_adj->set_lower(-(scaled_width/2));
            h_adj->set_upper(scaled_width/2);
            if(allocated_width > scaled_width){
                h_adj->set_page_size(scaled_width);
            }else{
                h_adj->set_page_size(allocated_width);
            }
            //calculate_coordinates();

        }

        if(!v_empty){
            v_adj->set_lower(-(scaled_height/2));
            v_adj->set_upper(scaled_height/2);
            if(allocated_height > scaled_height){
                v_adj->set_page_size(scaled_height);
            }else{
                v_adj->set_page_size(allocated_height);
            }
            //calculate_coordinates();     
        }
    }

    void calculate_coordinates(){
        //if(h_adj->get_page_size() >= (h_adj->get_upper()-h_adj->get_lower())){
        //    set_x_center(0);
        //}else{
            set_x_center(std::round(h_adj->get_value()+(h_adj->get_page_size()/2)));\
        //}

        //if(v_adj->get_page_size() >= (v_adj->get_upper()-v_adj->get_lower())){            
        //    set_y_center(0);
        //}else{
            set_y_center(std::round(v_adj->get_value()+(v_adj->get_page_size()/2)));
        //}
        queue_draw();
    }

    void calculate_coordinates_without_drawing(){
        /*if(scaling_factor == 1){
            set_x_center(0);
            set_y_center(0);
        }else{
            set_x_center(std::round(h_adj->get_value()+(h_adj->get_page_size()/2)));
            set_y_center(std::round(v_adj->get_value()+(v_adj->get_page_size()/2)));
        }
        //if(h_adj->get_page_size() >= (h_adj->get_upper()-h_adj->get_lower())){
        //    set_x_center(0);
        //}else{
            set_x_center(std::round(h_adj->get_value()+(h_adj->get_page_size()/2)));
        //}

        //if(v_adj->get_page_size() >= (v_adj->get_upper()-v_adj->get_lower())){            
        //    set_y_center(0);
        //}else{
            set_y_center(std::round(v_adj->get_value()+(v_adj->get_page_size()/2)));
        //}
    }*/

    /*void set_h_adjustment(Glib::RefPtr<Adjustment> a){
        h_adj = a;
        h_adj->set_value(0);

        h_empty = false;
        update_adjustments();
        h_adj->signal_value_changed().connect([&](){
            calculate_coordinates();
            //std::cout << "x " << std::round(h_adj->get_value()+(h_adj->get_page_size()/2)) << std::endl;
        });
    }

    void set_v_adjustment(Glib::RefPtr<Adjustment> a){
        v_adj = a;
        v_adj->set_value(0);

        v_empty = false;
        update_adjustments();
        v_adj->signal_value_changed().connect([&](){
            calculate_coordinates();
            //std::cout << "y " << std::round(v_adj->get_value()+(v_adj->get_page_size()/2)) << std::endl;
        });   
    }*/


    //Change the offset by percentage
    //expected parameter range (0 : 100)
    /*void set_x_center(int x){
        x_center = x;
        queue_draw();
    }

    void set_y_center(int y){
        y_center = y;
        queue_draw();
    }

    /*void set_x_range_widget(Range &x_scale){
        //x_scale = &x_range;
        x_scale.set_range(-(original_image->get_width())/2 , original_image->get_width()/2);
        x_scale.signal_change_value().connect([&](ScrollType type , double value){
            int factor = scaled_width / allocated_width;
            set_x_center(x_scale.get_value()*factor);
            return true;
        });
    }

    void set_y_range_widget(Range &y_scale){
        //y_scale = &y_range;
        y_scale.set_range(-(original_image->get_height())/2 , original_image->get_height()/2);
        y_scale.signal_change_value().connect([&](ScrollType type , double value){
            int factor = scaled_height / allocated_height;
            set_y_center(y_scale.get_value()*factor);
            return true;
        });
    }*/
    
    /*Picture(Glib::RefPtr<Gdk::Pixbuf> pixbuf){
        original_image = pixbuf;
        empty = false;
    }
    void set_pixbuf(Glib::RefPtr<Gdk::Pixbuf> pixbuf){
        original_image = pixbuf;
        empty = false;
    }
    const Glib::RefPtr<Gdk::Pixbuf> get_pixbuf(){
        return original_image;
    }

    void set_from_file(std::string name){
        original_image = Gdk::Pixbuf::create_from_file(name);
        empty = false;
    }

    virtual bool on_draw(const Cairo::RefPtr<Cairo::Context> &context){
        //const int WIDTH = this->get_allocation().get_width();
        //const int HEIGHT = this->get_allocation().get_height();
        allocated_width = this->get_allocation().get_width();
        allocated_height = this->get_allocation().get_height();
        if(!empty){
            //int width , height;
            set_dimensions();//(scaled_width , scaled_height , allocated_width , allocated_height);
            //update_adjustments();

            image = original_image->scale_simple(scaled_width , scaled_height , Gdk::INTERP_BILINEAR);
            //Gdk::Cairo::set_source_pixbuf(context , image , (WIDTH-width)/2 - x_offset_percentage*width , (HEIGHT-height)/2 - y_offset_percentage*height);
            //calculate_coordinates_without_drawing();
            
            //x_center & y_center = 0 , for cairo it's real_x_center = width/2
            //(ie: conversion from a graph containing -X & -Y to a graph containing only +ve coordinates)
            //calculate_coordinates();
            int real_x_center = (allocated_width/2) + x_center;
            int real_y_center = (allocated_height/2) + y_center;

            int x_corner = real_x_center - (scaled_width/2);
            int y_corner = real_y_center - (scaled_height/2);
            Gdk::Cairo::set_source_pixbuf(context , image , x_corner , y_corner);
            static int i = 0;
            std::cout << i << "- x " << x_center << " y " << y_center << std::endl;
            i++;
            context->paint();
        }else{
            context->set_source_rgba(0 , 0 , 0 , 0);
            context->paint();
        }
    }
};*/
