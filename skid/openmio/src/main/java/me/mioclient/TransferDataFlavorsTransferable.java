package me.mioclient;

import java.awt.Image;
import java.awt.datatransfer.DataFlavor;
import java.awt.datatransfer.Transferable;
import java.awt.datatransfer.UnsupportedFlavorException;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/TransferDataFlavorsTransferable.class */
public class TransferDataFlavorsTransferable implements Transferable {
    public Image image;

    public TransferDataFlavorsTransferable(Image image) {
        this.image = image;
    }

    public DataFlavor[] getTransferDataFlavors() {
        return new DataFlavor[]{DataFlavor.imageFlavor};
    }

    public boolean isDataFlavorSupported(DataFlavor dataFlavor) {
        return DataFlavor.imageFlavor.equals(dataFlavor);
    }

    public Object getTransferData(DataFlavor dataFlavor) throws java.awt.datatransfer.UnsupportedFlavorException {
        if (DataFlavor.imageFlavor.equals(dataFlavor)) {
            return this.image;
        }
        throw new UnsupportedFlavorException(dataFlavor);
    }
}
