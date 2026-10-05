document.addEventListener("DOMContentLoaded", () => {
    const urlParams = new URLSearchParams(window.location.search);
    if (urlParams.has("error")) {
        const errorBanner = document.getElementById("error-banner");
        if (errorBanner) {
            errorBanner.style.display = "block";
        }
    }
});
