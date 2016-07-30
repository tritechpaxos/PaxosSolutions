$(function () {
    var ev_src = new EventSource('/event/error'); // XXX script name
    ev_src.onmessage = function(e) {
	ent = $('<div class="alert alert-danger" role="alert">' + JSON.parse(e.data) + '</div>').prependTo($("#error-messages")).delay(10000).fadeOut();
	setTimeout(function() { ent.remove(); }, 12000);
    };
});
