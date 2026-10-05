package me.mioclient;

import com.mojang.brigadier.Command;
import com.mojang.brigadier.RedirectModifier;
import com.mojang.brigadier.context.CommandContext;
import com.mojang.brigadier.suggestion.Suggestions;
import com.mojang.brigadier.suggestion.SuggestionsBuilder;
import com.mojang.brigadier.tree.CommandNode;
import java.util.Iterator;
import java.util.concurrent.CompletableFuture;

/* compiled from: 0.java */
/* loaded from: mio-yarn.jar:me/mioclient/LiteralArgumentBuilder.class */
public class LiteralArgumentBuilder<S> extends com.mojang.brigadier.builder.LiteralArgumentBuilder<S> {

    /* compiled from: 0.java */
    /* loaded from: mio-yarn.jar:me/mioclient/LiteralArgumentBuilder$LiteralCommandNode.class */
    public static class LiteralCommandNode<S> extends com.mojang.brigadier.tree.LiteralCommandNode<S> {
        public LiteralCommandNode(String str, Command<S> command, java.util.function.Predicate<S> predicate, CommandNode<S> commandNode, RedirectModifier<S> redirectModifier, boolean z) {
            super(str, command, predicate, commandNode, redirectModifier, z);
        }

        public CompletableFuture<Suggestions> listSuggestions(CommandContext<S> commandContext, SuggestionsBuilder suggestionsBuilder) {
            return Suggestions.empty();
        }
    }

    public LiteralArgumentBuilder(String str) {
        super(str);
    }

    /* renamed from: build, reason: merged with bridge method [inline-methods] */
    public com.mojang.brigadier.tree.LiteralCommandNode<S> m296build() {
        LiteralCommandNode literalCommandNode = new LiteralCommandNode(getLiteral(), getCommand(), getRequirement(), getRedirect(), getRedirectModifier(), isFork());
        Iterator it = getArguments().iterator();
        while (it.hasNext()) {
            literalCommandNode.addChild((CommandNode) it.next());
        }
        return literalCommandNode;
    }
}
