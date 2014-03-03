elm = require('elm');

GadCon.init = function(gadget, name, id, style) {
    gadget.elements = {
        le_label: elm.Label({
            label: name
        }),
        le_button: elm.Button({
            label: 'btn'
        }),
    }
};

GadCon.menu_obj = [
    { label: 'Foo' },
    {
        label: 'Bar',
        check: 1,
        callback: function() {
            print('clicked ' + this.label + '; ' + this.check);
            this.check = !this.check;
        }
    },
    {},
    {
        label: 'Baz',
        items: [
            {
                label: 'Bla',
                check: 1
            },
            {
              label: 'Deep',
              items: [
                  { label: 'Hierarchy!' }
              ]
            }
        ]
    },
    { label: 'Has submenu', items: [ { label: 'Bla' } ] }
];

GadCon.menu = function(name, id) {
    return GadCon.menu_obj;
}
