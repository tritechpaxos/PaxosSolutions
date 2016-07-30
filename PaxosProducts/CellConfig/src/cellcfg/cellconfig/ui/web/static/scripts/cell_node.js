function update_cnode_status(cnid, cnode, row) {
    $(row).removeClass('danger').removeClass('success');
    if (cnode && cnode.pid) {
	$(row).addClass('success');
        $(row).find('td#cnode-pid-' + cnid).text(cnode.pid);
        $(row).find('td#cnode-consensus-' + cnid).text(cnode.consensus);
        $(row).find('td#cnode-birth-' + cnid).text(cnode.birth);
        if (cnode.ismaster) {
          $(row).find('td#cnode-ismaster-' + cnid).find('.glyphicon').addClass('show').removeClass('hidden');
        } else {
          $(row).find('td#cnode-ismaster-' + cnid).find('.glyphicon').addClass('hidden').removeClass('show');
        }
        $(row).find('td[id^="cnode-connection"]').each(function(idx, el) {
            if (cnode.connection[idx]) {
                $(el).find('.glyphicon-link').addClass('show').removeClass('hidden');
                $(el).find('.glyphicon-minus').addClass('hidden').removeClass('show');
            } else {
                $(el).find('.glyphicon-link').addClass('hidden').removeClass('show');
                $(el).find('.glyphicon-minus').addClass('show').removeClass('hidden');
            }
        });
        $(row).find('[id^="btn-start"]').addClass('disabled');
	var total = 0;
	var running = 0;
	$('table#cnodes tbody tr').each(function(idx, el) {
	    total++;
	    if ($(el).hasClass('success')) {
		running++;
	    }
	});
	if (running - 1 > total / 2) {
	    $('table#cnodes [id^="btn-stop"]').each(function(idx, el) {
		$(el).removeClass('disabled');
	    });
	}
    } else {
        $(row).addClass('danger');
        $(row).find('td#cnode-pid-' + cnid).text('―');
        $(row).find('td#cnode-consensus-' + cnid).text('―');
        $(row).find('td#cnode-birth-' + cnid).text('―');
        $(row).find('td#cnode-ismaster-' + cnid).find('.glyphicon').addClass('hidden').removeClass('show');
        $(row).find('td[id^="cnode-connection"]').each(function(idx, el) {
            $(el).find('.glyphicon-link').addClass('hidden').removeClass('show');
            $(el).find('.glyphicon-minus').addClass('show').removeClass('hidden');
        });
        $(row).find('[id^="btn-start"]').removeClass('disabled');
        $(row).find('[id^="btn-stop"]').addClass('disabled');
    }
}

function refresh_cnode_table(tgt, data) {
    $(tgt).find('tr').each(function(idx, row) {
        if (idx in data) {
            var ele_id = $(row).find('td:first').attr('id');
            var pos0 = ele_id.indexOf('-');
            var pos1 = ele_id.lastIndexOf('-');
            var cnid = ele_id.substr(pos1 + 1);
            update_cnode_status(cnid, data[idx], row);
        }
    });
}

function update_cell_buttons(tgt) {
    var stopped = 0;
    var total = 0;
    $(tgt).find('tr.danger').each(function(idx, row) {
	stopped++;
    });
    $(tgt).find('tr').each(function(idx, row) {
	total++;
    });
    if (stopped == total) {
	$('#btn-edit-cell').removeClass('disabled');
	$('#btn-delete-cell').removeClass('disabled');
    }
}

function request_cnode_op(tgt) {
    var ele_id = $(tgt).attr('id');
    var pos0 = ele_id.indexOf('-');
    var pos1 = ele_id.lastIndexOf('-');
    var op = ele_id.substring(pos0 + 1, pos1);
    var row = $(tgt).parents('tr:first');
    var no = $(row).find('td:first').text()
    $(row).find('.btn').addClass('disabled');
    $.ajax({
        url: "/api" + $(location).attr('pathname') + "/node/" + no + "/process",
        method: "PUT",
        contentType: "application/json; charset=utf-8",
        data: '{"operation": "' + op +'", "level": 1}',
        cache: false,
        success: function(data, status) {
          console.log(data);
	  if ('cell_node' in data) {
            update_cnode_status(cnid, data['cell_node'], row);
          } else {
            update_cnode_status(cnid, null, row);
          }
        },
    });
}

function request_cell_op(op) {
    $.ajax({
        url: "/api" + $(location).attr('pathname'),
        method: "PUT",
        contentType: "application/json; charset=utf-8",
        data: '{"op": "' + op +'", "level": 1}',
        cache: false,
    });
}

$(function () {
    $(':button[id^="btn-start"]').on('click', function(ev) {
	var tgt = ev.target;
	if ($(tgt).hasClass('disabled')) {
	    return;
	}
	request_cnode_op(tgt);
    });
    $(':button[id^="btn-stop"]').on('click', function(ev) {
	var tgt = ev.target;
	if ($(tgt).hasClass('disabled')) {
	    return;
	}
	$('table#cnodes :button[id^="btn-stop"]').each(function(idx, el) {
	    $(el).addClass('disabled');
	});
	request_cnode_op(tgt);
    });
    $(':button#btn-cell-start').on('click', function(ev) {
	var tgt = ev.target;
	if ($(tgt).hasClass('disabled')) {
	    return;
	}
	request_cell_op('start');
    });
    $(':button#btn-cell-stop').on('click', function(ev) {
	var tgt = ev.target;
	if ($(tgt).hasClass('disabled')) {
	    return;
	}
	request_cell_op('stop');
    });
    $(':button#show-conf').on('click', function(ev) {
        var paths = $(location).attr('pathname').split('/');
        var path1 = paths.splice(paths.length - 2).join('/');
        var path0 = paths.join('/');
        $.ajax({
	    url: path0 + '/api/' + path1,
	    accepts: { paxosconf: 'text/x.paxos-cell-conf' },
	    converters: {
		'text paxosconf': function(result) { return result; },
	    },
	    dataType: 'paxosconf',
	}).done(function(data) {
	    var dialog = $("#confModal");
	    $(dialog).find(".modal-body").html('<pre>' + data + '</pre>');
	    $(dialog).modal('show');
        });
    });
    $('#deploy-conf').on('click', function(ev) {
        var paths = $(location).attr('pathname').split('/');
        var path1 = paths.splice(paths.length - 2).join('/');
        var path0 = paths.join('/');
        $.ajax({
          url: path0 + '/api/' + path1,
          method: "PUT",
          contentType: "application/json; charset=utf-8",
          data: '{"op": "deploy config"}',
        });
    });
    var paths = $(location).attr('pathname').split('/');
    var path1 = paths.splice(paths.length - 2).join('/');
    var ev_src = new EventSource('/event/' + path1);
    ev_src.onmessage = function(e) {
	var data = JSON.parse(e.data);
	var tgt = $('table#cnodes').children('tbody:first');
	refresh_cnode_table(tgt, data);
	update_cell_buttons(tgt);
    }
 
});
