public class MessageTemperature implements java.io.Serializable {
    public final static byte MESURE = 0;
    public final static byte CHAUFFER = 1;

    protected String piece;
    protected int valeur;
    protected byte type;

    public MessageTemperature(int v, byte t, String p) {
        this.valeur = v; this.type = t; this.piece = p;
    }

    // Sérialisation : Objet -> Octets
    public byte[] toBytes() {
        byte[] tabPiece = piece.getBytes();
        byte[] tab = new byte[5 + tabPiece.length];
        tab[0] = (byte)(valeur & 0xff);
        tab[1] = (byte)((valeur >> 8) & 0xff);
        tab[2] = (byte)((valeur >> 16) & 0xff);
        tab[3] = (byte)((valeur >> 24) & 0xff);
        tab[4] = type;
        System.arraycopy(tabPiece, 0, tab, 5, tabPiece.length);
        return tab;
    }

    // Désérialisation : Octets -> Objet
    public static MessageTemperature fromBytes(byte[] tab, int length) {
        int valeur = (tab[0] & 0xFF) | ((tab[1] & 0xFF) << 8) | 
                     ((tab[2] & 0xFF) << 16) | ((tab[3] & 0xFF) << 24);
        String piece = new String(tab, 5, length - 5);
        return new MessageTemperature(valeur, tab[4], piece);
    }
    public String getPiece(){
        return piece;
    }
    public int getValeur(){
        return valeur;
    }
    public byte getType(){
        return type;
    }
}