function refresh_servers(tgt, data) {
    if (tgt.length > 0) {
	var rows = $(tgt).find('tr#srv-' + data['no']);
	if (rows.length > 0) {
	    $(rows).each(function(idx, row) {
		if (data['status'] == 'running') {
		    $(row).removeClass('danger').addClass('success');
		    $(row).find('#btn-rassrv-start-' + data['no'])
			.addClass('disabled');
		} else if (data['status'] == 'stopped') {
		    $(row).removeClass('success').addClass('danger');
		    $(row).find('#btn-rassrv-start-' + data['no'])
			.removeClass('disabled');
		}
	    });
	} else {
	    location.reload();
	}
    } else {
	location.reload();
    }
}


function refresh_targets(tgt, data) {
    $(tgt).find('tr#tgt-' + data['id']).each(function(idx, row) {
	var lbl ='';
	if (data['status'] == 'running') {
            $(row).removeClass('warning danger').addClass('success');
	    lbl = '正常 (' + data['running'] + '/' + data['total'] + ')';
	} else if (data['status'] == 'stopped') {
            $(row).removeClass('success warning').addClass('danger');
	    lbl = '停止 (' + data['running'] + '/' + data['total'] + ')';
	} else if (data['status'] == 'warning') {
            $(row).removeClass('success danger').addClass('warning');
	    lbl = '一部停止 (' + data['running'] + '/' + data['total'] + ')';
	} else if (data['status'] == 'error') {
            $(row).removeClass('success warning').addClass('danger');
	    lbl = '異常';
	} else {
            $(row).removeClass('success warning danger');
	    lbl = '不明';
	}
	$(row).find('td#tgt-status-' + data['id']).each(function(idx, ent) {
	    $(ent).text(lbl);
	});
    });
}

$(function () {
    var paths = $(location).attr('pathname').split('/');
    var tid = paths[paths.length - 1];
    var req = '/event/ras';
    var idx = true;
    if (tid != 'rasclstr') {
	req += '/' + tid;
	idx = false;
    }
    var ev_src = new EventSource(req);
    ev_src.onmessage = function(e) {
	data = JSON.parse(e.data);
	switch(data['etype']) {
	case 'target':
	    refresh_targets($('table#ras-target'), data);
	    break;
	case 'server':
	    if (!idx) {
		refresh_servers($('table#ras-server'), data);
	    }
	    break;
	}
    }
    $(':button[id^="btn-rassrv-start-"]').on('click', function(ev) {
	var tgt = ev.target;
	if ($(tgt).hasClass('disabled')) {
	    return;
	}
        var paths = $(location).attr('pathname').split('/');
	var btnid = $(tgt).attr('id').split('-');
	var no = btnid[btnid.length - 1];
	var url = "/api/ras/" + paths[paths.length - 1] + '/server/' +
	    no + '/process';
	$.ajax({
	    url: url,
	    method: "PUT",
	    contentType: "application/json; charset=utf-8",
	    data: '{"operation": "start"}',
	    cache: false,
	});
    });
    var srv_count = $("table#ras-server tbody tr").size();
    if (srv_count > 1) {
	$('table#ras-server').find('a[id^="btn-rassrv-delete-"]')
	    .each(function(idx, btn) {
		$(btn).removeClass('disabled');
	});
    }
})

