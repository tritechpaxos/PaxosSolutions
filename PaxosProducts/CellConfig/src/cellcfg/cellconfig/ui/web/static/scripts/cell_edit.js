var prev_node = '';
$("select[name$=-node]").on( "click", function (ev) {
        prev_node = $(ev.target).children('option:selected').text();
    }).change(function (ev) {
        var host = $(ev.target).children('option:selected').text();
        $(ev.target).parents('.panel:first').find('input[name$=_addr]')
          .map(function() {
            var curr_node = $(this).val()
            if (curr_node == '' || curr_node == prev_node) {
                $(this).val(host)
            }
        });
    }).change();

function update_cell_node (idx, elm) {
     function replace_nodes_id(i, val) {
         return val.replace(/nodes-\d+/, "nodes-" + idx);
     }
     $(elm).attr('id', replace_nodes_id);
     $(elm).find('[id^="nodes-"]').each(function() {
         $(this).attr('id', replace_nodes_id);
     });
     $(elm).find('[name^="nodes-"]').each(function() {
         $(this).attr('name', replace_nodes_id);
     });
     $(elm).find('[for^="nodes-"]').each(function() {
         $(this).attr('for', replace_nodes_id);
     });
     $(elm).find('.panel-title').each(function() {
         $(this).text(function (i, val) {
             return val.replace(/server \d+/, "server " + (idx + 1));
         });
     });
}
function update_cell_node_btn () {
    var sz = $( '.panel[id^="nodes-"]' ).length;
    if (sz <= 3) {
        var $delbtn = $( 'button[id*="delnd"]' );
        $delbtn.attr("disabled", "disabled");
        $delbtn.attr("type", "button");
        $( 'button[id*="addnd"]' ).attr("type", "button");
    } else {
        $( 'button[id*="delnd"]' ).removeAttr("disabled");
        $( 'button[id*="addnd"]' ).attr("type", "button");
    }
}

$('button[id*="addnd"]').on("click", function () {
     var $col = $(this).parents('.panel:first').parent();
     var $new_col = $col.clone(true);
     $new_col.find('input[id$="node_id"]').remove();
     $new_col.find('input[id$="_addr"]').val('');
     $new_col.insertAfter($col);
     $col.parent().find('.panel').each(update_cell_node);
     update_cell_node_btn();
});
$('button[id*="delnd"]').on("click", function () {
     var $col = $(this).parents('.panel:first').parent();
     var $prnt = $col.parent();
     $col.remove();
     $prnt.find('.panel').each(update_cell_node);
     update_cell_node_btn();
});
$(function () {
     update_cell_node_btn();
});
