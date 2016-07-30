function refresh_cell_table(tgt, data) {
    $(tgt).find('tr#' + data['id']).each(function(idx, row) {
      $(row).removeClass('success').removeClass('warning').removeClass('danger').removeClass('active');
      var icon = $(row).find('span.glyphicon:first');
      if (data['status'] == 'running') {
        $(row).addClass('success');
        $(icon).replaceWith('<span class="glyphicon glyphicon-ok"></span>');
      } else if (data['status'] == 'warning') {
        $(row).addClass('warning');
        $(icon).replaceWith('<span class="glyphicon glyphicon-warning-sign"></span>');
      } else if (data['status'] == 'error') {
        $(row).addClass('danger');
        $(icon).replaceWith('<span class="glyphicon glyphicon-alert"></span>');
      } else if (data['status'] == 'stopped') {
        $(row).addClass('active');
        $(icon).replaceWith('<span class="glyphicon glyphicon-remove"></span>');
      }
    });
}

$(function () {
    var ev_src = new EventSource('/event/cell');
    ev_src.onmessage = function(e) {
	data = JSON.parse(e.data);
	console.log(data);
	refresh_cell_table($('table#cell_list'), data);
    }

    $(':button[id^="btn-cell-start-"]').on('click', function(ev) {
	var tgt = ev.target;
	if (! $(tgt).hasClass('disabled')) {
            var id = $(tgt).parents('tr:first').attr('id');
	    $.ajax({
		url: "/api/cell/" + id,
		method: "PUT",
		contentType: "application/json; charset=utf-8",
		data: '{"op": "start", "level": 1}',
		cache: false,
	    });
	}
    });
 
    $(':button.paxos-show-config').on('click', function(ev) {
	var tgt = ev.target;
	if ($(tgt).hasClass('disabled')) {
	    return;
	}
        var row = $(tgt).parents('tr:first');
        var id = $(row).attr('id');
        $.ajax({
	    url: '/api/cell/' + id, // XXX script name
	    accepts: { paxosconf: 'text/x.paxos-cell-conf' },
	    converters: {
		'text paxosconf': function(result) { return result; },
	    },
	    dataType: 'paxosconf',
	}).done(function(data) {
	    var title = $(row).children('td:nth-child(2)').text();
	    var dialog = $("#confModal");
	    $(dialog).find(".modal-body").html('<pre>' + data + '</pre>');
	    $(dialog).modal('show');
	    $(dialog).find(".modal-title").text(title);
	});
    });
});
