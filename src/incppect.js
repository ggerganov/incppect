    //src_render: null,

    //this.src_render = this.render.toString().match(/function[^{]+\{([\s\S]*)\}$/)[1];
    //document.getElementById('src_render').value = this.src_render;

    //var this_ref = this;
    //document.getElementById('src_render').addEventListener('keydown', function (e){
    //    if ((e.ctrlKey || e.metaKey) && (e.keyCode == 13 || e.keyCode == 10)) {
    //        this_ref.render = new Function(this.value);
    //    }
    //}, false);

    var incppect = {
        // websocket data
        ws: null,
        ws_uri: 'ws://' + window.location.hostname + ':' + window.location.port + '/incppect',

        // vars data
        nvars: 0,
        vars_map: {},
        var_to_id: {},
        id_to_var: {},

        // requests data
        requests: [],
        requests_old: [],
        requests_new_vars: false,
        requests_regenerate: true,

        // timestamps
        t_start_ms: null,
        t_frame_begin_ms: null,
        t_requests_last_update_ms: null,

        // constants
        k_var_delim: ' ',
        k_requests_update_freq_ms: 50,

        timestamp: function() {
            return window.performance && window.performance.now && window.performance.timing &&
                window.performance.timing.navigationStart ? window.performance.now() + window.performance.timing.navigationStart : Date.now();
        },

        init: function() {
            var onopen = this.onopen.bind(this);
            var onclose = this.onclose.bind(this);
            var onmessage = this.onmessage.bind(this);
            var onerror = this.onerror.bind(this);

            this.ws = new WebSocket(this.ws_uri);
            this.ws.binaryType = 'arraybuffer';
            this.ws.onopen = function(evt) { onopen(evt) };
            this.ws.onclose = function(evt) { onclose(evt) };
            this.ws.onmessage = function(evt) { onmessage(evt) };
            this.ws.onerror = function(evt) { onerror(evt) };

            this.t_start_ms = this.timestamp();
            this.t_requests_last_update_ms = this.timestamp() - this.k_requests_update_freq_ms;

            window.requestAnimationFrame(this.loop.bind(this));
        },

        loop: function() {
            if (this.ws == null) {
                setTimeout(this.init.bind(this), 1000);
                return;
            }

            if (this.ws.readyState !== this.ws.OPEN) {
                window.requestAnimationFrame(this.loop.bind(this));
                return;
            }

            this.t_frame_begin_ms = this.timestamp();
            this.requests_regenerate = this.t_frame_begin_ms - this.t_requests_last_update_ms > this.k_requests_update_freq_ms;

            if (this.requests_regenerate) {
                this.requests_old = this.requests;
                this.requests = [];

                try {
                    this.render();
                } catch(err) {
                    this.onerror('Failed to render state: ' + err);
                }
            }

            if (this.requests_regenerate) {
                if (this.requests_new_vars) {
                    this.send_var_to_id_map();
                    this.requests_new_vars = false;
                }
                this.send_requests();
                this.t_requests_last_update_ms = this.timestamp();
            }

            window.requestAnimationFrame(this.loop.bind(this));
        },

        get: function(path, ...args) {
            for (var i = 1; i < arguments.length; i++) {
                path = path.replace('%d', arguments[i]);
            }

            if (!(path in this.vars_map)) {
                this.vars_map[path] = new ArrayBuffer();
                this.var_to_id[path] = this.nvars;
                this.id_to_var[this.nvars] = path;
                ++this.nvars;

                this.requests_new_vars = true;
            }

            if (this.requests_regenerate) {
                this.requests.push(this.var_to_id[path]);
            }

            return this.vars_map[path];
        },

        get_int8: function(path, ...args) {
            return this.get_int16_arr(path, ...args)[0];
        },

        get_int8_arr: function(path, ...args) {
            var abuf = this.get(path, ...args);
            return new Int8Array(abuf);
        },

        get_uint8: function(path, ...args) {
            return this.get_int16_arr(path, ...args)[0];
        },

        get_uint8_arr: function(path, ...args) {
            var abuf = this.get(path, ...args);
            return new Uint8Array(abuf);
        },

        get_int16: function(path, ...args) {
            return this.get_int16_arr(path, ...args)[0];
        },

        get_int16_arr: function(path, ...args) {
            var abuf = this.get(path, ...args);
            return new Int16Array(abuf);
        },

        get_uint16: function(path, ...args) {
            return this.get_int16_arr(path, ...args)[0];
        },

        get_uint16_arr: function(path, ...args) {
            var abuf = this.get(path, ...args);
            return new Uint16Array(abuf);
        },

        get_int32: function(path, ...args) {
            return this.get_int32_arr(path, ...args)[0];
        },

        get_int32_arr: function(path, ...args) {
            var abuf = this.get(path, ...args);
            return new Int32Array(abuf);
        },

        get_uint32: function(path, ...args) {
            return this.get_int32_arr(path, ...args)[0];
        },

        get_uint32_arr: function(path, ...args) {
            var abuf = this.get(path, ...args);
            return new Uint32Array(abuf);
        },

        get_float: function(path, ...args) {
            return this.get_float_arr(path, ...args)[0];
        },

        get_float_arr: function(path, ...args) {
            var abuf = this.get(path, ...args);
            return new Float32Array(abuf);
        },

        get_str: function(path, ...args) {
            var abuf = this.get(path, ...args);
            return String.fromCharCode.apply(null, new Uint8Array(abuf));
        },

        send_var_to_id_map: function() {
            var msg = '';
            var delim = this.k_var_delim;
            for (var key in this.var_to_id) {
                var nidxs = 0;
                var idxs = delim;
                var keyp = key.replace(/\[\d*\]/g, function(m) { ++nidxs; idxs += m.replace(/[\[\]]/g, '') + delim; return '[%d]'; });
                msg += keyp + delim + this.var_to_id[key].toString() + delim + nidxs + idxs;
            }
            var data = new Int8Array(4 + msg.length + 1);
            data[0] = 1;
            var enc = new TextEncoder();
            data.set(enc.encode(msg), 4);
            data[4 + msg.length] = 0;
            this.ws.send(data);
        },

        send_requests: function() {
            var same = true;
            if (this.requests_old === null || this.requests.length !== this.requests_old.length){
                same = false;
            } else {
                for (var i = 0; i < this.requests.length; ++i) {
                    if (this.requests[i] !== this.requests_old[i]) {
                        same = false;
                        break;
                    }
                }
            }

            if (same) {
                var data = new Int32Array(1);
                data[0] = 3;
                this.ws.send(data);
            } else {
                var data = new Int32Array(this.requests.length + 1);
                data.set(new Int32Array(this.requests), 1);
                data[0] = 2;
                this.ws.send(data);
            }
        },

        onopen: function(evt) {
        },

        onclose: function(evt) {
            this.nvars = 0;
            this.vars_map = {};
            this.var_to_id = {};
            this.id_to_var = {};
            this.requests = null;
            this.requests_old = null;
            this.ws = null;
        },

        onmessage: function(evt) {
            var offset = 0;
            var offset_new = 0;
            var int_view = new Int32Array(evt.data);
            var total_size = evt.data.byteLength;
            var id = 0;
            var len = 0;
            while (4*offset < total_size) {
                id = int_view[offset];
                len = int_view[offset + 1];
                offset += 2;
                offset_new = offset + len/4;
                this.vars_map[this.id_to_var[id]] = evt.data.slice(4*offset, 4*offset_new);
                offset = offset_new;
            }
        },

        onerror: function(evt) {
            console.error("[incppect]", evt);
        },

        render: function() {
        },

        //render_default: function() {
        //    var nclients = this.get_int32('incppect.nclients');
        //    var tx_total = this.get_int32('incppect.tx_total');
        //    var rx_total = this.get_int32('incppect.rx_total');

        //    this.output.innerHTML = 'nclients = ' + nclients;
        //    for (var i = 0; i < nclients; ++i) {
        //        var ipaddress_bytes = this.get_int32('incppect.ip_address[%d]', i);
        //        function int_to_ip(int) {
        //            var part1 = int & 255;
        //            var part2 = ((int >> 8) & 255);
        //            var part3 = ((int >> 16) & 255);
        //            var part4 = ((int >> 24) & 255);

        //            return part1 + "." + part2 + "." + part3 + "." + part4;
        //        }
        //        this.output.innerHTML += '<br>client ' + i + ' : ' + int_to_ip(ipaddress_bytes);
        //    }
        //    this.output.innerHTML += '<br>tx total = ' + (tx_total/1024.0/1024.0).toFixed(4) + ' MB';
        //    this.output.innerHTML += '<br>rx total = ' + (rx_total/1024.0/1024.0).toFixed(4) + ' MB';
        //    this.output.innerHTML += '<br>';
        //},
    }
