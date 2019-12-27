function generateUrl(form) {
    var reason = "";
    reason +=  form.url.value;
    reason += "?";
    reason += "ip="+form.ip.value;
    reason += "&sir="+form.sir.value;
    reason += "&sit="+form.sit.value;
    reason += "&csit="+form.csit.value;
    reason += "&clrit="+form.clrit.value;
    reason += "&shortcut="+form.shortcut.value;
    reason += "&debug="+form.debug.value;

    console.log(reason);

    copyToClipboard(reason);
}



function copyToClipboard(text) {
    var container = document.createElement("textarea");
    // to avoid breaking orgain page when copying more words
    // cant copy when adding below this code
    // dummy.style.display = 'none'
    document.body.appendChild(container);
    //Be careful if you use texarea. setAttribute('value', value), which works with "input" does not work with "textarea". – Eduard
    container.value = text;
    container.select();
    container.setSelectionRange(0, 99999); /*For mobile devices*/
    document.execCommand("copy");
    document.body.removeChild(container);

    alert("Copied the text to your clipboard");
}