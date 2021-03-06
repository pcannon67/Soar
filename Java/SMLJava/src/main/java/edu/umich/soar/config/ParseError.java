package edu.umich.soar.config;

public class ParseError extends RuntimeException
{
    private static final long serialVersionUID = 1L;

    public ParseError(String msg, int lineNumber, String line)
    {
        super("Parse error: " + msg + "\nNear line " + lineNumber + ": " + line);
    }
}
