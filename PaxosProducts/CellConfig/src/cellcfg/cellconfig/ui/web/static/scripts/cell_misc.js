$(function () {
    var paths = $(location).attr('pathname').split('/');
    var tgt = null;
    $.each($(location).attr('search').substr(1).split('&'), function(idx,val) {
      var kv = val.split('=');
      if (kv[0] == 'target') { tgt = kv[1]; }
    });

    $.ajax({
        url: '/api/cell/' + paths[2] + '/misc/' + tgt,
    }).done(function(data) {
        $.each(data, function(idx0, val0) {
            var tr = $( "<tr></tr>" ).appendTo(
                $('table[id="misc_info"]').children('tbody:first'));
            $.each(val0, function(idx, val) {
                $(tr).append("<td>"+val+"</td>");
            });
        });
    }).always(function() {
        $('table[id="misc_info"]').css("background-image", "none");
    });
});
