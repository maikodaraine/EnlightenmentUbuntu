elm = require('elm');
icons = require('icons');

var EXPAND_BOTH = { x : 1.0, y : 1.0 };
var FILL_BOTH = { x : -1.0, y : -1.0 };
var WIN_WIDTH = 320;
var WIN_HEIGHT = 480;

var prefs_window = new elm.Window({
    title : "Prefs Demo",
    width : WIN_WIDTH,
    height : WIN_HEIGHT,
    elements : {
        the_background : elm.Background({
            weight : EXPAND_BOTH,
            align : FILL_BOTH,
            resize : true,
        }),
        prefs : elm.Prefs({
            weight : EXPAND_BOTH,
            align : FILL_BOTH,
            resize : true,
            autosave: true,
            file : {
                'name' : 'prefs.epb'
            },
            data : {
                'name' : 'prefs.cfg',
                'mode' : 'read_write'
            },
            on_item_changed: function (item) {
               switch (item)
                  {
                     case 'main:size_x':
                       grid.item_size_vertical = Number(prefs.items[item]);
                       break;
                     case 'main:size_y':
                       grid.item_size_horizontal = Number(prefs.items[item]);
                       break;
                     default:
                       grid.clear();
                       grid.elements = getElements();
                  }
            },
            on_page_changed: function (page) {
                print('Page loaded:', page);
            },
            on_action: function (action) {
                print('Action', action);
            },
        })
    }
});

var prefs = prefs_window.elements.prefs;

function getElements() {

    var item_class = {
        text: function(part) { return this.data; },
        content: function(part) {
            if (part == 'elm.swallow.icon')
                return elm.Icon({ image: this.data });
        },
    };

    var gengrid_item = function(image) {
        return { 'data' : image, 'class' : item_class };
    };

    var icon_list = [];

    if (prefs.items['main:icon_elm'] == true)
        icon_list = icon_list.concat(icons.elementary);

    if (prefs.items['main:icon_freedsk'] == true)
        icon_list = icon_list.concat(icons.freedesktop);

    var elements = [];
    var filter = prefs.items['main:filter'];

    for (var i in icon_list) {
        var icon = icon_list[i];
        if (icon.indexOf(filter) >= 0)
            elements.push(gengrid_item(icon));
    }

    return elements;
}

var grid_window = new elm.Window({
  title: 'Gengrid',
  width: WIN_WIDTH,
  height: WIN_HEIGHT,
  elements: {
    background: elm.Background({
      expand: 'both',
      fill: 'both',
      resize: true
    }),
    box: elm.Box({
      expand: 'both',
      fill: 'both',
      resize: true,
      elements: {
        grid: elm.Gengrid({
          expand: 'both',
          fill: 'both',
          item_size_vertical: Number(prefs.items['main:size_x']),
          item_size_horizontal: Number(prefs.items['main:size_y']),
          elements: getElements(),
        })
      },
      on_delete: function() {
        print('On delete');
        elm.exit();
      }
    })
  }
});

var grid = grid_window.elements.box.elements.grid;
