var elm = require('elm');
var icons = require('icons');

var window = elm.realise(elm.Window({
  title: 'Gengrid',
  width: 480,
  height: 480,
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
          item_size_horizontal: 128,
          item_size_vertical: 128,
          expand: 'both',
          fill: 'both',
          elements: {}
        }),
        tool: elm.Toolbar({
          fill: 'both',
          select_mode: 'none',
          elements: [
            {
              icon: 'delete',
              label: 'Delete',
              on_select: function() {
                var items = [];
                for (var i in grid_elements)
                  if (grid_elements[i].data.check)
                    items.push(grid_elements[i]);
                window.elements.box.elements.grid.clear(items);
              }
            }
          ]
        })
      }
    })
  }
}));

var gengrid_item = function(image) {
  return {
    'data': {
        image: image,
        check: false
    },
    'class' : {
      style: 'default',
      text: function(part) {
        return this.data.image;
      },
      content: function(part) {
        if (part == 'elm.swallow.icon')
          return elm.Icon({ lookup_order: 'fdo,theme', image: this.data.image });
        if (part == 'elm.swallow.end')
          return elm.Check({
              propagate_events: false,
              state: this.data.check,
              on_change: function() {
                this.data.check = !this.data.check;
              }.bind(this)
          });
      },
      state: function(part) {
        return false
      }
    },
    on_select: function(item) {
      print("Selected " + this.data + " icon.");
    }
  };
};

var grid_elements = window.elements.box.elements.grid.elements;

for (var i = 0; i < icons.elementary.length; ++i)
  grid_elements[i] = gengrid_item(icons.elementary[i]);

for (var i = 0; i < icons.freedesktop.length; ++i)
  grid_elements[i + icons.elementary.length] = gengrid_item(icons.freedesktop[i]);
