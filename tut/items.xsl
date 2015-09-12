<?xml version="1.0" encoding="utf-8"?>
<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">
  <xsl:template match="items">
    <h3><xsl:value-of select="./@name" /></h3>
    <ul>
      <xsl:apply-templates select="./item"><xsl:with-param name="mode">head</xsl:with-param></xsl:apply-templates>
    </ul>
      <hr />
      <xsl:apply-templates select="./item"><xsl:with-param name="mode">body</xsl:with-param></xsl:apply-templates>
  </xsl:template>

  <xsl:template match="item">
    <xsl:param name="mode" /><xsl:param name="parent" />
    <xsl:if test="$mode='head' or $mode=''">
      <li><a href="#{@name}"><xsl:value-of select="@name" /></a></li>
      <xsl:if test="count(./item) > 0">
	<ul>
	  <xsl:apply-templates select="./item"><xsl:with-param name="mode">head</xsl:with-param></xsl:apply-templates>
	</ul>
	</xsl:if>
    </xsl:if>
    <xsl:if test="$mode='body' or mode=''">
      <div style="padding-left: 15px; border-left:solid 2px #ccc">
	<a name="{@name}" />
	<b><xsl:if test="$parent and @brief!='yes'"><xsl:value-of select="$parent" />/</xsl:if><xsl:value-of select="@name" /></b>:  <br /><!--<xsl:apply-templates select="text()|./*" /><br /><br />-->
	<p><xsl:apply-templates select="text()|./*">
	  <xsl:with-param name="parent"><xsl:value-of select="concat(concat($parent,'/'),@name)" /></xsl:with-param>
	  <xsl:with-param name="mode">body</xsl:with-param>
	</xsl:apply-templates></p>
      </div>
    </xsl:if>

  </xsl:template>

</xsl:stylesheet>
