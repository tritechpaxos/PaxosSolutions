function request_node_op(tgt, op) {
    var ele_id = $(tgt).attr('id');
    var nid = ele_id.substr(ele_id.lastIndexOf('-') + 1);
    var row = $(tgt).parents('tr:first');
    $(row).find('.btn').addClass('disabled');
    $.ajax({
        url: "/api/node/" + nid + "/" + op,
        method: "PUT",
        cache: false
    }).always(function(data) {
        $(row).find('.btn').removeClass('disabled');
    });
}


$(function () {
    $('button[id^="btn-deploy"]').on('click', function(ev) {
	var tgt = ev.target;
	if ($(tgt).hasClass("disabled")) {
	    return;
	}
	request_node_op(ev.target, 'deployment');
    });
});

