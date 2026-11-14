(function () {
  'use strict';

  var API = '/api/publishing/';
  var WAYS = [['api', 'HA API'], ['mqtt', 'MQTT'], ['web', 'Web']];
  var GROUP_DEFAULT = 255;
  var offset = 0;
  var kind = '';
  var group = '';
  var data = null;
  var groupsOpen = false;
  var view, errorLine, kindSelect, groupSelect;

  function call(method, op, params, cb) {
    var parts = [];
    for (var k in params || {}) {
      if (params[k] !== undefined && params[k] !== null && params[k] !== '') parts.push(encodeURIComponent(k) + '=' + encodeURIComponent(params[k]));
    }
    var xhr = new XMLHttpRequest();
    xhr.open(method, API + op + (parts.length ? '?' + parts.join('&') : ''), true);
    xhr.onload = function () {
      var body = null;
      try {
        body = JSON.parse(xhr.responseText);
      } catch (e) {
        body = null;
      }
      cb(xhr.status >= 200 && xhr.status < 300 && body !== null, body, xhr.status);
    };
    xhr.onerror = function () {
      cb(false, null, 0);
    };
    xhr.send();
  }

  function el(tag, css, text) {
    var e = document.createElement(tag);
    if (css) e.style.cssText = css;
    if (text !== undefined) e.textContent = text;
    return e;
  }

  var BTN = 'padding:3px 8px;border-radius:5px;border:0;background:rgba(127,127,127,.25);color:inherit;cursor:pointer;margin-left:4px';
  var BTN_MAIN = 'padding:3px 8px;border-radius:5px;border:0;background:#44739e;color:#fff;cursor:pointer;margin-left:4px';
  var CELL = 'text-align:left;padding:6px 8px;border-bottom:1px solid rgba(127,127,127,.25)';

  function button(text, main, action) {
    var b = el('button', main ? BTN_MAIN : BTN, text);
    b.addEventListener('click', action);
    return b;
  }

  function showError(text) {
    errorLine.textContent = text || '';
  }

  function load() {
    call('GET', 'list', { offset: offset, kind: kind, group: group }, function (ok, body, status) {
      if (!ok) {
        showError('no answer from the device (HTTP ' + status + ')');
        return;
      }
      data = body;
      if (offset > 0 && offset >= body.count) {
        offset = 0;
        load();
        return;
      }
      render();
    });
  }

  function act(op, params) {
    call('POST', op, params, function (ok, body, status) {
      if (!ok || !body.success) showError(body && body.error ? body.error : 'no answer from the device (HTTP ' + status + ')');
      else showError('');
      load();
    });
  }

  function label(text) {
    text = String(text).replace(/_/g, ' ');
    return text.charAt(0).toUpperCase() + text.slice(1);
  }

  function toggle(on, action) {
    var s = el('span', 'display:inline-block;width:34px;height:18px;border-radius:9px;position:relative;vertical-align:middle;cursor:pointer;background:' + (on ? '#44739e' : 'rgba(127,127,127,.45)'));
    s.appendChild(el('span', 'position:absolute;top:2px;width:14px;height:14px;border-radius:50%;background:#fff;left:' + (on ? '18px' : '2px')));
    s.addEventListener('click', action);
    return s;
  }

  function fillSelect(select, first, items, value) {
    select.textContent = '';
    var all = el('option', '', first);
    all.value = '';
    select.appendChild(all);
    items.forEach(function (item) {
      var o = el('option', '', item[1]);
      o.value = String(item[0]);
      select.appendChild(o);
    });
    select.value = String(value);
  }

  function renderGroups() {
    var box = el('div');
    data.groups.forEach(function (g, i) {
      var row = el('div', 'display:flex;gap:6px;align-items:center;padding:4px 0');
      var input = el('input', 'width:200px');
      input.value = g.name;
      input.maxLength = 22;
      row.appendChild(input);
      row.appendChild(button('Rename', false, function () {
        act('group', { op: 'rename', id: g.id, name: input.value });
      }));
      if (i > 0) row.appendChild(button('Up', false, function () {
        act('group', { op: 'move', id: g.id, by: -1 });
      }));
      if (i + 1 < data.groups.length) row.appendChild(button('Down', false, function () {
        act('group', { op: 'move', id: g.id, by: 1 });
      }));
      row.appendChild(button('Remove', false, function () {
        act('group', { op: 'remove', id: g.id });
      }));
      box.appendChild(row);
    });
    var add = el('div', 'display:flex;gap:6px;align-items:center;padding:8px 0 0');
    var name = el('input', 'width:200px');
    name.placeholder = 'new group';
    name.maxLength = 22;
    add.appendChild(name);
    add.appendChild(button('Add', true, function () {
      if (name.value) act('group', { op: 'add', name: name.value });
    }));
    box.appendChild(add);
    return box;
  }

  function renderTable() {
    var ways = WAYS.filter(function (w) {
      return data.ways[w[0]] !== undefined;
    });
    var table = el('table', 'border-collapse:collapse;width:100%;font-size:14px');
    var head = el('tr');
    head.appendChild(el('th', CELL + ';font-size:12px;opacity:.7', 'Entity'));
    head.appendChild(el('th', CELL + ';font-size:12px;opacity:.7', 'Kind'));
    ways.forEach(function (w) {
      var th = el('th', CELL + ';font-size:12px;opacity:.7', w[1]);
      th.appendChild(button('all', false, function () {
        act('all', { way: data.ways[w[0]], on: 1, kind: kind, group: group });
      }));
      th.appendChild(button('none', false, function () {
        act('all', { way: data.ways[w[0]], on: 0, kind: kind, group: group });
      }));
      head.appendChild(th);
    });
    if (data.ways.web !== undefined) head.appendChild(el('th', CELL + ';font-size:12px;opacity:.7', 'Web group'));
    table.appendChild(head);
    data.entities.forEach(function (e) {
      var row = el('tr');
      row.appendChild(el('td', CELL, e.name));
      row.appendChild(el('td', CELL + ';opacity:.7', label(e.kind)));
      ways.forEach(function (w) {
        var bit = data.ways[w[0]];
        var on = (e.off & bit) === 0;
        var td = el('td', CELL);
        td.appendChild(toggle(on, function () {
          act('set', { kind: e.kind, key: e.key, off: on ? e.off | bit : e.off & ~bit });
        }));
        row.appendChild(td);
      });
      if (data.ways.web !== undefined) {
        var cell = el('td', CELL);
        var select = el('select');
        fillSelect(select, 'Default', data.groups.map(function (g) {
          return [g.id, g.name];
        }), e.group === GROUP_DEFAULT ? '' : e.group);
        select.addEventListener('change', function () {
          act('set', { kind: e.kind, key: e.key, group: select.value === '' ? GROUP_DEFAULT : select.value });
        });
        cell.appendChild(select);
        row.appendChild(cell);
      }
      table.appendChild(row);
    });
    var box = el('div');
    box.appendChild(table);
    var foot = el('div', 'display:flex;gap:6px;align-items:center;margin-top:8px;font-size:12px');
    var last = Math.min(data.offset + data.entities.length, data.count);
    foot.appendChild(el('span', 'opacity:.7', data.count ? (data.offset + 1) + ' - ' + last + ' of ' + data.count : 'no entity'));
    if (data.offset > 0) foot.appendChild(button('Previous', false, function () {
      offset = Math.max(0, offset - data.page);
      load();
    }));
    if (last < data.count) foot.appendChild(button('Next', false, function () {
      offset += data.page;
      load();
    }));
    box.appendChild(foot);
    return box;
  }

  function render() {
    fillSelect(kindSelect, 'All kinds', data.kinds.map(function (k) {
      return [k, label(k)];
    }), kind);
    fillSelect(groupSelect, 'All groups', [[GROUP_DEFAULT, 'Default']].concat(data.groups.map(function (g) {
      return [g.id, g.name];
    })), group);
    groupSelect.style.display = data.ways.web !== undefined ? '' : 'none';
    view.textContent = '';
    view.appendChild(groupsOpen ? renderGroups() : renderTable());
  }

  function build() {
    var card = el('div', 'margin:1em 0;background:rgba(127,127,127,.08);border-radius:12px;padding:10px 12px');
    card.id = 'esp-publishing';
    var head = el('div', 'display:flex;gap:8px;align-items:center;margin-bottom:8px;flex-wrap:wrap');
    head.appendChild(el('b', 'font-size:16px', 'Publishing'));
    kindSelect = el('select', 'margin-left:12px');
    kindSelect.addEventListener('change', function () {
      kind = kindSelect.value;
      offset = 0;
      load();
    });
    head.appendChild(kindSelect);
    groupSelect = el('select');
    groupSelect.addEventListener('change', function () {
      group = groupSelect.value;
      offset = 0;
      load();
    });
    head.appendChild(groupSelect);
    var groupsButton = button('Groups', false, function () {
      groupsOpen = !groupsOpen;
      groupsButton.textContent = groupsOpen ? 'Entities' : 'Groups';
      if (data) render();
    });
    groupsButton.style.marginLeft = 'auto';
    head.appendChild(groupsButton);
    head.appendChild(button('Reset', false, function () {
      act('reset', { kind: kind, group: group });
    }));
    card.appendChild(head);
    view = el('div');
    card.appendChild(view);
    errorLine = el('div', 'margin-top:8px;color:#e57373;font-size:12px');
    card.appendChild(errorLine);
    return card;
  }

  function attach(card) {
    if (window.ESPHFE_TABS && window.ESPHFE_TABS.add('publishing', 'Publishing', card)) {
      card.style.margin = '0';
      return;
    }
    var app = document.querySelector('esp-app');
    function mount() {
      var root = app && app.shadowRoot;
      var anchor = root && root.querySelector('esp-entity-table');
      if (!anchor || !anchor.parentNode) return false;
      anchor.parentNode.insertBefore(card, anchor.nextSibling);
      return true;
    }
    if (!mount()) {
      setTimeout(function () {
        if (!mount()) (app ? app.parentNode : document.body).appendChild(card);
      }, 300);
    }
  }

  function start() {
    attach(build());
    load();
  }

  if (document.readyState === 'loading') document.addEventListener('DOMContentLoaded', start);
  else start();
})();
